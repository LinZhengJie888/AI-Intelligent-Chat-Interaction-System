/**
 * @file GroupService.cpp
 * @brief 群聊管理服务模块实现
 */

#include "friend/GroupService.h"
#include "module/Database.h"
#include "model/GroupChat.h"
#include "model/GroupMember.h"
#include "model/GroupRequest.h"
#include "model/ChatRecord.h"
#include "common/Util.h"
#include <iostream>
#include <sstream>
#include <cstdio>

/**
 * @brief JSON转义辅助函数
 */
static std::string escapeJson(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
}

/**
 * @brief 构造函数
 */
GroupService::GroupService(Database& db) : db_(db) {}

/**
 * @brief 析构函数
 */
GroupService::~GroupService() {
    // 清理资源
}

/**
 * @brief 初始化服务
 */
bool GroupService::init() {
    if (!createGroupChatTable()) {
        std::cerr << "Failed to create group_chat table" << std::endl;
        return false;
    }
    
    if (!createGroupMemberTable()) {
        std::cerr << "Failed to create group_member table" << std::endl;
        return false;
    }
    
    if (!createGroupRequestTable()) {
        std::cerr << "Failed to create group_request table" << std::endl;
        return false;
    }
    
    if (!createGroupRecordTable()) {
        std::cerr << "Failed to create group_record table" << std::endl;
        return false;
    }
    
    std::cout << "GroupService initialized successfully" << std::endl;
    return true;
}

/**
 * @brief 创建群聊
 */
bool GroupService::createGroup(const std::string& creator_id, const std::string& group_name, 
                               std::string& group_id) {
    group_id = generateGroupId();
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_chat (group_id, group_name, creator_id, create_time) "
             "VALUES ('%s', '%s', '%s', NOW())",
             group_id.c_str(), group_name.c_str(), creator_id.c_str());
    
    if (!db_.execute(sql)) {
        std::cerr << "Failed to create group" << std::endl;
        return false;
    }
    
    // 将创建者添加为群主
    if (!addGroupMember(group_id, creator_id, 2)) {
        std::cerr << "Failed to add creator as group member" << std::endl;
        return false;
    }
    
    std::cout << "Group created: " << group_id << " by " << creator_id << std::endl;
    return true;
}

/**
 * @brief 发送加群申请
 */
int GroupService::sendJoinRequest(const std::string& user_id, const std::string& group_id, 
                                  const std::string& request_msg) {
    // 检查是否已是群成员
    if (isGroupMember(user_id, group_id)) {
        return -2;  // 已是成员
    }
    
    // 检查申请是否已存在
    char check_sql[512];
    snprintf(check_sql, sizeof(check_sql),
             "SELECT id FROM group_request "
             "WHERE from_user_id='%s' AND group_id='%s' AND status=0",
             user_id.c_str(), group_id.c_str());
    
    MYSQL_RES* check_res = db_.query(check_sql);
    if (check_res) {
        MYSQL_ROW check_row = mysql_fetch_row(check_res);
        if (check_row) {
            db_.freeResult(check_res);
            return -3;  // 申请已存在
        }
        db_.freeResult(check_res);
    }
    
    // 插入加群申请
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_request (group_id, from_user_id, request_msg, status, create_time) "
             "VALUES ('%s', '%s', '%s', 0, NOW())",
             group_id.c_str(), user_id.c_str(), request_msg.c_str());
    
    if (!db_.execute(sql)) {
        std::cerr << "Failed to insert group request" << std::endl;
        return -1;
    }
    
    std::cout << "Group join request sent from " << user_id << " to " << group_id << std::endl;
    return 0;
}

/**
 * @brief 同意加群申请
 */
bool GroupService::agreeJoinRequest(const std::string& user_id, const std::string& group_id, 
                                   const std::string& operator_id) {
    // 检查操作者是否是群主
    if (!isGroupCreator(operator_id, group_id)) {
        std::cerr << "Operator is not group creator" << std::endl;
        return false;
    }
    
    // 更新申请状态
    if (!updateRequestStatus(user_id, group_id, 1)) {
        return false;
    }
    
    // 添加群成员
    if (!addGroupMember(group_id, user_id, 0)) {
        return false;
    }
    
    std::cout << "Group join request agreed: " << user_id << " to " << group_id << std::endl;
    return true;
}

/**
 * @brief 拒绝加群申请
 */
bool GroupService::rejectJoinRequest(const std::string& user_id, const std::string& group_id, 
                                     const std::string& operator_id) {
    // 检查操作者是否是群主
    if (!isGroupCreator(operator_id, group_id)) {
        std::cerr << "Operator is not group creator" << std::endl;
        return false;
    }
    
    // 更新申请状态
    if (!updateRequestStatus(user_id, group_id, 2)) {
        return false;
    }
    
    std::cout << "Group join request rejected: " << user_id << " from " << group_id << std::endl;
    return true;
}

/**
 * @brief 发送群消息
 */
bool GroupService::sendMessage(const std::string& sender_id, const std::string& group_id, 
                               const std::string& content) {
    // 检查是否是群成员
    if (!isGroupMember(sender_id, group_id)) {
        std::cerr << "User is not group member" << std::endl;
        return false;
    }
    
    // 保存消息到聊天记录表
    char sql[4096];
    snprintf(sql, sizeof(sql),
             "INSERT INTO chat_record (sender_id, group_id, content, send_time, is_ai) "
             "VALUES ('%s', '%s', '%s', NOW(), 0)",
             sender_id.c_str(), group_id.c_str(), content.c_str());
    
    if (!db_.execute(sql)) {
        std::cerr << "Failed to save group message" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 获取群聊信息
 */
std::string GroupService::getGroupInfo(const std::string& group_id) {
    GroupInfo info = getGroupInfoStruct(group_id);
    
    std::ostringstream oss;
    oss << "{\"group_id\":\"" << info.group_id << "\""
        << ",\"group_name\":\"" << escapeJson(info.group_name) << "\""
        << ",\"creator_id\":\"" << info.creator_id << "\""
        << ",\"avatar_path\":\"" << escapeJson(info.avatar_path) << "\""
        << ",\"announcement\":\"" << escapeJson(info.announcement) << "\""
        << ",\"create_time\":\"" << info.create_time << "\""
        << ",\"member_count\":" << info.member_count
        << "}";
    
    return oss.str();
}

/**
 * @brief 获取群聊信息（结构体形式）
 */
GroupInfo GroupService::getGroupInfoStruct(const std::string& group_id) {
    GroupInfo info;
    info.group_id = group_id;
    info.member_count = 0;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT group_id, group_name, creator_id, avatar_path, announcement, create_time "
             "FROM group_chat WHERE group_id='%s'",
             group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return info;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row) {
        info.group_id = row[0] ? row[0] : "";
        info.group_name = row[1] ? row[1] : "";
        info.creator_id = row[2] ? row[2] : "";
        info.avatar_path = row[3] ? row[3] : "";
        info.announcement = row[4] ? row[4] : "";
        info.create_time = row[5] ? row[5] : "";
    }
    
    db_.freeResult(res);
    
    // 获取成员数量
    char count_sql[256];
    snprintf(count_sql, sizeof(count_sql),
             "SELECT COUNT(*) FROM group_member WHERE group_id='%s'",
             group_id.c_str());
    
    MYSQL_RES* count_res = db_.query(count_sql);
    if (count_res) {
        MYSQL_ROW count_row = mysql_fetch_row(count_res);
        if (count_row && count_row[0]) {
            info.member_count = atoi(count_row[0]);
        }
        db_.freeResult(count_res);
    }
    
    return info;
}

/**
 * @brief 获取群成员列表
 */
std::string GroupService::getGroupMembers(const std::string& group_id) {
    std::vector<GroupMemberInfo> members = getGroupMembersStruct(group_id);
    
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < members.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"user_id\":\"" << members[i].user_id << "\""
            << ",\"username\":\"" << escapeJson(members[i].username) << "\""
            << ",\"nickname\":\"" << escapeJson(members[i].nickname) << "\""
            << ",\"avatar_path\":\"" << escapeJson(members[i].avatar_path) << "\""
            << ",\"role\":" << static_cast<int>(members[i].role)
            << ",\"join_time\":\"" << members[i].join_time << "\""
            << "}";
    }
    oss << "]";
    
    return oss.str();
}

/**
 * @brief 获取群成员列表（结构体形式）
 */
std::vector<GroupMemberInfo> GroupService::getGroupMembersStruct(const std::string& group_id) {
    std::vector<GroupMemberInfo> members;
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT gm.user_id, gm.role, gm.join_time, "
             "u.username, u.nickname, u.avatar_path "
             "FROM group_member gm "
             "JOIN user u ON gm.user_id = u.user_id "
             "WHERE gm.group_id='%s' "
             "ORDER BY gm.role DESC, gm.join_time ASC",
             group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return members;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupMemberInfo info;
        info.user_id = row[0] ? row[0] : "";
        info.role = row[1] ? static_cast<int8_t>(atoi(row[1])) : 0;
        info.join_time = row[2] ? row[2] : "";
        info.username = row[3] ? row[3] : "";
        info.nickname = row[4] ? row[4] : "";
        info.avatar_path = row[5] ? row[5] : "";
        members.push_back(info);
    }
    
    db_.freeResult(res);
    return members;
}

/**
 * @brief 获取群成员列表（用于ChatService广播）
 */
std::vector<GroupMember> GroupService::getGroupMembersList(const std::string& group_id) {
    std::vector<GroupMember> members;
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, group_id, user_id, role, join_time "
             "FROM group_member WHERE group_id='%s'",
             group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return members;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupMember member;
        member.id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
        member.group_id = row[1] ? strtoull(row[1], nullptr, 10) : 0;
        member.user_id = row[2] ? strtoull(row[2], nullptr, 10) : 0;
        member.role = row[3] ? static_cast<int8_t>(atoi(row[3])) : 0;
        member.join_time = row[4] ? row[4] : "";
        members.push_back(member);
    }
    
    db_.freeResult(res);
    return members;
}

/**
 * @brief 获取用户加入的群聊列表
 */
std::string GroupService::getUserGroups(const std::string& user_id) {
    std::vector<GroupInfo> groups = getUserGroupsStruct(user_id);
    
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < groups.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"group_id\":\"" << groups[i].group_id << "\""
            << ",\"group_name\":\"" << escapeJson(groups[i].group_name) << "\""
            << ",\"avatar_path\":\"" << escapeJson(groups[i].avatar_path) << "\""
            << ",\"member_count\":" << groups[i].member_count
            << ",\"create_time\":\"" << groups[i].create_time << "\""
            << "}";
    }
    oss << "]";
    
    return oss.str();
}

/**
 * @brief 获取用户加入的群聊列表（结构体形式）
 */
std::vector<GroupInfo> GroupService::getUserGroupsStruct(const std::string& user_id) {
    std::vector<GroupInfo> groups;
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT gc.group_id, gc.group_name, gc.creator_id, gc.avatar_path, "
             "gc.announcement, gc.create_time "
             "FROM group_chat gc "
             "JOIN group_member gm ON gc.group_id = gm.group_id "
             "WHERE gm.user_id='%s' "
             "ORDER BY gc.create_time DESC",
             user_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return groups;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupInfo info;
        info.group_id = row[0] ? row[0] : "";
        info.group_name = row[1] ? row[1] : "";
        info.creator_id = row[2] ? row[2] : "";
        info.avatar_path = row[3] ? row[3] : "";
        info.announcement = row[4] ? row[4] : "";
        info.create_time = row[5] ? row[5] : "";
        info.member_count = 0;
        
        // 获取成员数量
        char count_sql[256];
        snprintf(count_sql, sizeof(count_sql),
                 "SELECT COUNT(*) FROM group_member WHERE group_id='%s'",
                 info.group_id.c_str());
        
        MYSQL_RES* count_res = db_.query(count_sql);
        if (count_res) {
            MYSQL_ROW count_row = mysql_fetch_row(count_res);
            if (count_row && count_row[0]) {
                info.member_count = atoi(count_row[0]);
            }
            db_.freeResult(count_res);
        }
        
        groups.push_back(info);
    }
    
    db_.freeResult(res);
    return groups;
}

/**
 * @brief 获取待处理的加群申请列表
 */
std::string GroupService::getPendingRequests(const std::string& group_id) {
    std::vector<GroupRequestInfo> requests = getPendingRequestsStruct(group_id);
    
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < requests.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"request_id\":" << requests[i].request_id
            << ",\"group_id\":\"" << requests[i].group_id << "\""
            << ",\"group_name\":\"" << escapeJson(requests[i].group_name) << "\""
            << ",\"from_user_id\":\"" << requests[i].from_user_id << "\""
            << ",\"from_username\":\"" << escapeJson(requests[i].from_username) << "\""
            << ",\"from_avatar\":\"" << escapeJson(requests[i].from_avatar) << "\""
            << ",\"request_msg\":\"" << escapeJson(requests[i].request_msg) << "\""
            << ",\"status\":" << static_cast<int>(requests[i].status)
            << ",\"create_time\":\"" << requests[i].create_time << "\""
            << "}";
    }
    oss << "]";
    
    return oss.str();
}

/**
 * @brief 获取待处理的加群申请列表（结构体形式）
 */
std::vector<GroupRequestInfo> GroupService::getPendingRequestsStruct(const std::string& group_id) {
    std::vector<GroupRequestInfo> requests;
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT gr.id, gr.group_id, gr.from_user_id, gr.request_msg, "
             "gr.status, gr.create_time, gc.group_name, u.username, u.avatar_path "
             "FROM group_request gr "
             "JOIN group_chat gc ON gr.group_id = gc.group_id "
             "JOIN user u ON gr.from_user_id = u.user_id "
             "WHERE gr.group_id='%s' AND gr.status=0 "
             "ORDER BY gr.create_time DESC",
             group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return requests;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        GroupRequestInfo info;
        info.request_id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
        info.group_id = row[1] ? row[1] : "";
        info.from_user_id = row[2] ? row[2] : "";
        info.request_msg = row[3] ? row[3] : "";
        info.status = row[4] ? static_cast<int8_t>(atoi(row[4])) : 0;
        info.create_time = row[5] ? row[5] : "";
        info.group_name = row[6] ? row[6] : "";
        info.from_username = row[7] ? row[7] : "";
        info.from_avatar = row[8] ? row[8] : "";
        requests.push_back(info);
    }
    
    db_.freeResult(res);
    return requests;
}

/**
 * @brief 获取群聊创建者
 */
std::string GroupService::getGroupCreator(const std::string& group_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT creator_id FROM group_chat WHERE group_id='%s'",
             group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return "";
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    std::string creator_id;
    if (row && row[0]) {
        creator_id = row[0];
    }
    
    db_.freeResult(res);
    return creator_id;
}

/**
 * @brief 检查用户是否是群成员
 */
bool GroupService::isGroupMember(const std::string& user_id, const std::string& group_id) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id FROM group_member WHERE user_id='%s' AND group_id='%s'",
             user_id.c_str(), group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    bool exists = (row != nullptr);
    db_.freeResult(res);
    
    return exists;
}

/**
 * @brief 检查用户是否是群主
 */
bool GroupService::isGroupCreator(const std::string& user_id, const std::string& group_id) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id FROM group_chat WHERE group_id='%s' AND creator_id='%s'",
             group_id.c_str(), user_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    bool exists = (row != nullptr);
    db_.freeResult(res);
    
    return exists;
}

/**
 * @brief 修改群名
 */
bool GroupService::modifyGroupName(const std::string& group_id, const std::string& new_name, 
                                   const std::string& operator_id) {
    // 检查操作者是否是群主或管理员
    char check_sql[512];
    snprintf(check_sql, sizeof(check_sql),
             "SELECT role FROM group_member WHERE user_id='%s' AND group_id='%s'",
             operator_id.c_str(), group_id.c_str());
    
    MYSQL_RES* check_res = db_.query(check_sql);
    if (!check_res) {
        return false;
    }
    
    MYSQL_ROW check_row = mysql_fetch_row(check_res);
    if (!check_row || !check_row[0]) {
        db_.freeResult(check_res);
        return false;
    }
    
    int role = atoi(check_row[0]);
    db_.freeResult(check_res);
    
    // 只有群主(2)或管理员(1)可以修改群名
    if (role < 1) {
        std::cerr << "Permission denied" << std::endl;
        return false;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE group_chat SET group_name='%s', update_time=NOW() WHERE group_id='%s'",
             new_name.c_str(), group_id.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 修改群头像
 */
bool GroupService::modifyGroupAvatar(const std::string& group_id, const std::string& avatar_path, 
                                     const std::string& operator_id) {
    // 检查操作者是否是群主或管理员
    char check_sql[512];
    snprintf(check_sql, sizeof(check_sql),
             "SELECT role FROM group_member WHERE user_id='%s' AND group_id='%s'",
             operator_id.c_str(), group_id.c_str());
    
    MYSQL_RES* check_res = db_.query(check_sql);
    if (!check_res) {
        return false;
    }
    
    MYSQL_ROW check_row = mysql_fetch_row(check_res);
    if (!check_row || !check_row[0]) {
        db_.freeResult(check_res);
        return false;
    }
    
    int role = atoi(check_row[0]);
    db_.freeResult(check_res);
    
    // 只有群主(2)或管理员(1)可以修改群头像
    if (role < 1) {
        std::cerr << "Permission denied" << std::endl;
        return false;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE group_chat SET avatar_path='%s', update_time=NOW() WHERE group_id='%s'",
             avatar_path.c_str(), group_id.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 退出群聊
 */
bool GroupService::leaveGroup(const std::string& user_id, const std::string& group_id) {
    // 群主不能退出群聊
    if (isGroupCreator(user_id, group_id)) {
        std::cerr << "Group creator cannot leave group" << std::endl;
        return false;
    }
    
    return removeGroupMember(group_id, user_id);
}

/**
 * @brief 踢出群成员
 */
bool GroupService::kickMember(const std::string& user_id, const std::string& group_id, 
                              const std::string& operator_id) {
    // 检查操作者权限
    char check_sql[512];
    snprintf(check_sql, sizeof(check_sql),
             "SELECT role FROM group_member WHERE user_id='%s' AND group_id='%s'",
             operator_id.c_str(), group_id.c_str());
    
    MYSQL_RES* check_res = db_.query(check_sql);
    if (!check_res) {
        return false;
    }
    
    MYSQL_ROW check_row = mysql_fetch_row(check_res);
    if (!check_row || !check_row[0]) {
        db_.freeResult(check_res);
        return false;
    }
    
    int operator_role = atoi(check_row[0]);
    db_.freeResult(check_res);
    
    // 检查被踢者角色
    char target_sql[512];
    snprintf(target_sql, sizeof(target_sql),
             "SELECT role FROM group_member WHERE user_id='%s' AND group_id='%s'",
             user_id.c_str(), group_id.c_str());
    
    MYSQL_RES* target_res = db_.query(target_sql);
    if (!target_res) {
        return false;
    }
    
    MYSQL_ROW target_row = mysql_fetch_row(target_res);
    if (!target_row || !target_row[0]) {
        db_.freeResult(target_res);
        return false;
    }
    
    int target_role = atoi(target_row[0]);
    db_.freeResult(target_res);
    
    // 权限检查：群主可以踢任何人，管理员只能踢普通成员
    if (operator_role < target_role) {
        std::cerr << "Permission denied" << std::endl;
        return false;
    }
    
    return removeGroupMember(group_id, user_id);
}

/**
 * @brief 创建群聊表
 */
bool GroupService::createGroupChatTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS group_chat ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "group_id VARCHAR(32) NOT NULL UNIQUE,"
        "group_name VARCHAR(64) NOT NULL,"
        "creator_id VARCHAR(32) NOT NULL,"
        "avatar_path VARCHAR(256) DEFAULT '',"
        "announcement TEXT,"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "INDEX idx_group_id (group_id),"
        "INDEX idx_creator_id (creator_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 创建群成员表
 */
bool GroupService::createGroupMemberTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS group_member ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "group_id VARCHAR(32) NOT NULL,"
        "user_id VARCHAR(32) NOT NULL,"
        "role TINYINT DEFAULT 0 COMMENT '0-member, 1-admin, 2-creator',"
        "join_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE KEY uk_group_user (group_id, user_id),"
        "INDEX idx_group_id (group_id),"
        "INDEX idx_user_id (user_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 创建加群请求表
 */
bool GroupService::createGroupRequestTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS group_request ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "group_id VARCHAR(32) NOT NULL,"
        "from_user_id VARCHAR(32) NOT NULL,"
        "request_msg VARCHAR(256) DEFAULT '',"
        "status TINYINT DEFAULT 0 COMMENT '0-pending, 1-agreed, 2-rejected',"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "INDEX idx_group_id (group_id),"
        "INDEX idx_from_user (from_user_id),"
        "INDEX idx_status (status)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 创建群聊记录表
 */
bool GroupService::createGroupRecordTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS group_record ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "group_id VARCHAR(32) NOT NULL,"
        "sender_id VARCHAR(32) NOT NULL,"
        "content TEXT NOT NULL,"
        "send_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "is_ai TINYINT DEFAULT 0,"
        "INDEX idx_group_id (group_id),"
        "INDEX idx_sender_id (sender_id),"
        "INDEX idx_send_time (send_time)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 生成唯一群聊ID
 */
std::string GroupService::generateGroupId() {
    return "G" + util::generateUUID().substr(0, 8);
}

/**
 * @brief 添加群成员
 */
bool GroupService::addGroupMember(const std::string& group_id, const std::string& user_id, int8_t role) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_member (group_id, user_id, role, join_time) "
             "VALUES ('%s', '%s', %d, NOW())",
             group_id.c_str(), user_id.c_str(), static_cast<int>(role));
    
    return db_.execute(sql);
}

/**
 * @brief 移除群成员
 */
bool GroupService::removeGroupMember(const std::string& group_id, const std::string& user_id) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "DELETE FROM group_member WHERE group_id='%s' AND user_id='%s'",
             group_id.c_str(), user_id.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 更新加群请求状态
 */
bool GroupService::updateRequestStatus(const std::string& user_id, const std::string& group_id, 
                                       int8_t status) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE group_request SET status=%d, update_time=NOW() "
             "WHERE from_user_id='%s' AND group_id='%s' AND status=0",
             static_cast<int>(status), user_id.c_str(), group_id.c_str());
    
    return db_.execute(sql);
}
