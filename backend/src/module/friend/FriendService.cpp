/**
 * @file FriendService.cpp
 * @brief 好友管理服务模块实现
 */

#include "friend/FriendService.h"
#include "module/Database.h"
#include "model/User.h"
#include "model/UserDAO.h"
#include "model/FriendRequest.h"
#include "model/FriendRelation.h"
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
 * @brief 根据用户ID字符串获取数字ID
 * @param db 数据库连接
 * @param user_id 用户ID字符串
 * @return 用户数字ID，失败返回0
 */
static uint64_t getUserIdNum(Database& db, const std::string& user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT id FROM user WHERE user_id='%s'", user_id.c_str());
    
    MYSQL_RES* res = db.query(sql);
    if (!res) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        db.freeResult(res);
        return 0;
    }
    
    uint64_t id = strtoull(row[0], nullptr, 10);
    db.freeResult(res);
    return id;
}

/**
 * @brief 构造函数
 */
FriendService::FriendService(Database& db) : db_(db) {}

/**
 * @brief 析构函数
 */
FriendService::~FriendService() {
    // 清理资源
}

/**
 * @brief 初始化服务
 */
bool FriendService::init() {
    if (!createFriendRelationTable()) {
        std::cerr << "Failed to create friend_relation table" << std::endl;
        return false;
    }
    
    if (!createFriendRequestTable()) {
        std::cerr << "Failed to create friend_request table" << std::endl;
        return false;
    }
    
    std::cout << "FriendService initialized successfully" << std::endl;
    return true;
}

/**
 * @brief 发送好友请求
 */
int FriendService::sendRequest(const std::string& from_user_id, const std::string& to_user_id, 
                               const std::string& request_msg) {
    // 检查是否是自己
    if (from_user_id == to_user_id) {
        return -1;  // 不能添加自己为好友
    }
    
    // 检查用户是否存在（获取数值主键）
    uint64_t from_id_num = getUserIdNum(db_, from_user_id);
    uint64_t to_id_num = getUserIdNum(db_, to_user_id);
    if (from_id_num == 0 || to_id_num == 0) {
        std::cerr << "User not found" << std::endl;
        return -1;
    }
    
    // 检查是否已是好友
    if (isFriend(from_user_id, to_user_id)) {
        return -2;  // 已是好友
    }
    
    // 检查请求是否已存在（使用数值主键）
    char check_sql[512];
    snprintf(check_sql, sizeof(check_sql),
             "SELECT id FROM friend_request "
             "WHERE from_user_id=%lu AND to_user_id=%lu AND status=0",
             (unsigned long)from_id_num, (unsigned long)to_id_num);
    
    MYSQL_RES* check_res = db_.query(check_sql);
    if (check_res) {
        MYSQL_ROW check_row = mysql_fetch_row(check_res);
        if (check_row) {
            db_.freeResult(check_res);
            return -3;  // 请求已存在
        }
        db_.freeResult(check_res);
    }
    
    // 检查冷却期
    if (isInCoolingPeriod(from_user_id, to_user_id)) {
        return -4;  // 在冷却期内
    }
    
    // 插入好友请求（使用数值主键）
    std::string safe_msg = db_.escapeString(request_msg);
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "INSERT INTO friend_request (from_user_id, to_user_id, request_msg, status, create_time) "
             "VALUES (%lu, %lu, '%s', 0, NOW())",
             (unsigned long)from_id_num, (unsigned long)to_id_num, safe_msg.c_str());
    
    if (!db_.execute(sql)) {
        std::cerr << "Failed to insert friend request" << std::endl;
        return -1;
    }
    
    std::cout << "Friend request sent from " << from_user_id << " to " << to_user_id << std::endl;
    return 0;
}

/**
 * @brief 同意好友请求
 */
bool FriendService::agreeRequest(const std::string& from_user_id, const std::string& to_user_id) {
    // 更新请求状态
    if (!updateRequestStatus(from_user_id, to_user_id, 1)) {
        return false;
    }
    
    // 添加好友关系（双向）
    if (!addFriendRelation(from_user_id, to_user_id)) {
        return false;
    }
    
    std::cout << "Friend request agreed: " << from_user_id << " and " << to_user_id << std::endl;
    return true;
}

/**
 * @brief 拒绝好友请求
 */
bool FriendService::rejectRequest(const std::string& from_user_id, const std::string& to_user_id) {
    // 更新请求状态
    if (!updateRequestStatus(from_user_id, to_user_id, 2)) {
        return false;
    }
    
    // 设置冷却期
    if (!setCoolingPeriod(from_user_id, to_user_id)) {
        std::cerr << "Failed to set cooling period" << std::endl;
        // 继续执行，不影响拒绝操作
    }
    
    std::cout << "Friend request rejected: " << from_user_id << " from " << to_user_id << std::endl;
    return true;
}

/**
 * @brief 删除好友
 */
bool FriendService::deleteFriend(const std::string& user_id, const std::string& friend_id) {
    if (!removeFriendRelation(user_id, friend_id)) {
        return false;
    }
    
    std::cout << "Friend deleted: " << user_id << " and " << friend_id << std::endl;
    return true;
}

/**
 * @brief 检查是否是好友关系
 */
bool FriendService::isFriend(const std::string& user_id1, const std::string& user_id2) {
    uint64_t id1_num = getUserIdNum(db_, user_id1);
    uint64_t id2_num = getUserIdNum(db_, user_id2);
    if (id1_num == 0 || id2_num == 0) {
        return false;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id FROM friend_relation "
             "WHERE (user_id=%lu AND friend_id=%lu) OR (user_id=%lu AND friend_id=%lu)",
             (unsigned long)id1_num, (unsigned long)id2_num,
             (unsigned long)id2_num, (unsigned long)id1_num);
    
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
 * @brief 获取好友列表
 */
std::string FriendService::getFriendList(const std::string& user_id) {
    std::vector<FriendInfo> friends = getFriendListStruct(user_id);
    
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < friends.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"user_id\":\"" << friends[i].user_id << "\""
            << ",\"username\":\"" << escapeJson(friends[i].username) << "\""
            << ",\"nickname\":\"" << escapeJson(friends[i].nickname) << "\""
            << ",\"avatar_path\":\"" << escapeJson(friends[i].avatar_path) << "\""
            << ",\"remark\":\"" << escapeJson(friends[i].remark) << "\""
            << ",\"add_time\":\"" << friends[i].add_time << "\""
            << "}";
    }
    oss << "]";
    
    return oss.str();
}

/**
 * @brief 获取好友列表（结构体形式）
 */
std::vector<FriendInfo> FriendService::getFriendListStruct(const std::string& user_id) {
    std::vector<FriendInfo> friends;
    
    uint64_t user_id_num = getUserIdNum(db_, user_id);
    if (user_id_num == 0) {
        return friends;
    }
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT fr.friend_id, fr.remark, fr.create_time, "
             "u.user_id, u.username, u.nickname, u.avatar_path "
             "FROM friend_relation fr "
             "JOIN user u ON fr.friend_id = u.id "
             "WHERE fr.user_id=%lu "
             "ORDER BY fr.create_time DESC",
             (unsigned long)user_id_num);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return friends;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        FriendInfo info;
        info.user_id = row[3] ? row[3] : "";
        info.username = row[4] ? row[4] : "";
        info.nickname = row[5] ? row[5] : "";
        info.avatar_path = row[6] ? row[6] : "";
        info.remark = row[1] ? row[1] : "";
        info.add_time = row[2] ? row[2] : "";
        friends.push_back(info);
    }
    
    db_.freeResult(res);
    return friends;
}

/**
 * @brief 获取待处理的好友请求列表
 */
std::string FriendService::getPendingRequests(const std::string& user_id) {
    std::vector<FriendRequestInfo> requests = getPendingRequestsStruct(user_id);
    
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < requests.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"request_id\":" << requests[i].request_id
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
 * @brief 获取待处理的好友请求列表（结构体形式）
 */
std::vector<FriendRequestInfo> FriendService::getPendingRequestsStruct(const std::string& user_id) {
    std::vector<FriendRequestInfo> requests;
    
    uint64_t user_id_num = getUserIdNum(db_, user_id);
    if (user_id_num == 0) {
        return requests;
    }
    
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "SELECT fr.id, u_from.user_id, u_to.user_id, fr.request_msg, "
             "fr.status, fr.create_time, fr.cooling_time, "
             "u_from.username, u_from.avatar_path "
             "FROM friend_request fr "
             "JOIN user u_from ON fr.from_user_id = u_from.id "
             "JOIN user u_to ON fr.to_user_id = u_to.id "
             "WHERE fr.to_user_id=%lu AND fr.status=0 "
             "ORDER BY fr.create_time DESC",
             (unsigned long)user_id_num);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return requests;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        FriendRequestInfo info;
        info.request_id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
        info.from_user_id = row[1] ? row[1] : "";
        info.to_user_id = row[2] ? row[2] : "";
        info.request_msg = row[3] ? row[3] : "";
        info.status = row[4] ? static_cast<int8_t>(atoi(row[4])) : 0;
        info.create_time = row[5] ? row[5] : "";
        info.cooling_time = row[6] ? row[6] : "";
        info.from_username = row[7] ? row[7] : "";
        info.from_avatar = row[8] ? row[8] : "";
        requests.push_back(info);
    }
    
    db_.freeResult(res);
    return requests;
}

/**
 * @brief 设置好友备注
 */
bool FriendService::setRemark(const std::string& user_id, const std::string& friend_id, 
                               const std::string& remark) {
    uint64_t user_id_num = getUserIdNum(db_, user_id);
    uint64_t friend_id_num = getUserIdNum(db_, friend_id);
    if (user_id_num == 0 || friend_id_num == 0) {
        return false;
    }
    
    std::string safe_remark = db_.escapeString(remark);
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE friend_relation SET remark='%s' "
             "WHERE user_id=%lu AND friend_id=%lu",
             safe_remark.c_str(), (unsigned long)user_id_num, (unsigned long)friend_id_num);
    
    return db_.execute(sql);
}

/**
 * @brief 获取好友数量
 */
int FriendService::getFriendCount(const std::string& user_id) {
    uint64_t user_id_num = getUserIdNum(db_, user_id);
    if (user_id_num == 0) {
        return 0;
    }
    
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT COUNT(*) FROM friend_relation WHERE user_id=%lu",
             (unsigned long)user_id_num);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return 0;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    int count = 0;
    if (row && row[0]) {
        count = atoi(row[0]);
    }
    
    db_.freeResult(res);
    return count;
}

/**
 * @brief 创建好友关系表
 */
bool FriendService::createFriendRelationTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS friend_relation ("
        "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "user_id BIGINT UNSIGNED NOT NULL,"
        "friend_id BIGINT UNSIGNED NOT NULL,"
        "remark VARCHAR(64) DEFAULT '',"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE KEY uk_user_friend (user_id, friend_id),"
        "INDEX idx_friend_id (friend_id)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 创建好友请求表
 */
bool FriendService::createFriendRequestTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS friend_request ("
        "id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,"
        "from_user_id BIGINT UNSIGNED NOT NULL,"
        "to_user_id BIGINT UNSIGNED NOT NULL,"
        "request_msg VARCHAR(256) DEFAULT '',"
        "status TINYINT DEFAULT 0 COMMENT '0-pending, 1-agreed, 2-rejected',"
        "cooling_time DATETIME DEFAULT NULL,"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "INDEX idx_from_user (from_user_id),"
        "INDEX idx_to_user (to_user_id),"
        "INDEX idx_status (status)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 检查冷却期
 */
bool FriendService::isInCoolingPeriod(const std::string& from_user_id, const std::string& to_user_id) {
    uint64_t from_id_num = getUserIdNum(db_, from_user_id);
    uint64_t to_id_num = getUserIdNum(db_, to_user_id);
    if (from_id_num == 0 || to_id_num == 0) {
        return false;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT cooling_time FROM friend_request "
             "WHERE from_user_id=%lu AND to_user_id=%lu AND status=2 "
             "ORDER BY create_time DESC LIMIT 1",
             (unsigned long)from_id_num, (unsigned long)to_id_num);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        db_.freeResult(res);
        return false;
    }
    
    // 解析冷却时间
    struct tm tm_time;
    strptime(row[0], "%Y-%m-%d %H:%M:%S", &tm_time);
    std::time_t cooling_time = mktime(&tm_time);
    
    db_.freeResult(res);
    
    std::time_t now = std::time(nullptr);
    return now < cooling_time;
}

/**
 * @brief 添加好友关系（双向）
 */
bool FriendService::addFriendRelation(const std::string& user_id1, const std::string& user_id2) {
    uint64_t id1_num = getUserIdNum(db_, user_id1);
    uint64_t id2_num = getUserIdNum(db_, user_id2);
    if (id1_num == 0 || id2_num == 0) {
        return false;
    }
    
    char sql1[512];
    char sql2[512];
    
    snprintf(sql1, sizeof(sql1),
             "INSERT INTO friend_relation (user_id, friend_id, create_time) "
             "VALUES (%lu, %lu, NOW())",
             (unsigned long)id1_num, (unsigned long)id2_num);
    
    snprintf(sql2, sizeof(sql2),
             "INSERT INTO friend_relation (user_id, friend_id, create_time) "
             "VALUES (%lu, %lu, NOW())",
             (unsigned long)id2_num, (unsigned long)id1_num);
    
    return db_.execute(sql1) && db_.execute(sql2);
}

/**
 * @brief 删除好友关系（双向）
 */
bool FriendService::removeFriendRelation(const std::string& user_id1, const std::string& user_id2) {
    uint64_t id1_num = getUserIdNum(db_, user_id1);
    uint64_t id2_num = getUserIdNum(db_, user_id2);
    if (id1_num == 0 || id2_num == 0) {
        return false;
    }
    
    char sql1[512];
    char sql2[512];
    
    snprintf(sql1, sizeof(sql1),
             "DELETE FROM friend_relation "
             "WHERE user_id=%lu AND friend_id=%lu",
             (unsigned long)id1_num, (unsigned long)id2_num);
    
    snprintf(sql2, sizeof(sql2),
             "DELETE FROM friend_relation "
             "WHERE user_id=%lu AND friend_id=%lu",
             (unsigned long)id2_num, (unsigned long)id1_num);
    
    return db_.execute(sql1) && db_.execute(sql2);
}

/**
 * @brief 更新好友请求状态
 */
bool FriendService::updateRequestStatus(const std::string& from_user_id, const std::string& to_user_id, 
                                         int8_t status) {
    uint64_t from_id_num = getUserIdNum(db_, from_user_id);
    uint64_t to_id_num = getUserIdNum(db_, to_user_id);
    if (from_id_num == 0 || to_id_num == 0) {
        return false;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE friend_request SET status=%d, update_time=NOW() "
             "WHERE from_user_id=%lu AND to_user_id=%lu AND status=0",
             static_cast<int>(status), (unsigned long)from_id_num, (unsigned long)to_id_num);
    
    return db_.execute(sql);
}

/**
 * @brief 设置冷却期
 */
bool FriendService::setCoolingPeriod(const std::string& from_user_id, const std::string& to_user_id) {
    uint64_t from_id_num = getUserIdNum(db_, from_user_id);
    uint64_t to_id_num = getUserIdNum(db_, to_user_id);
    if (from_id_num == 0 || to_id_num == 0) {
        return false;
    }
    
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE friend_request SET cooling_time=DATE_ADD(NOW(), INTERVAL %d HOUR) "
             "WHERE from_user_id=%lu AND to_user_id=%lu AND status=2",
             COOLING_PERIOD_HOURS, (unsigned long)from_id_num, (unsigned long)to_id_num);
    
    return db_.execute(sql);
}
