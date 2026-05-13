/**
 * @file GroupChatDAO.cpp
 * @brief 群聊数据访问对象实现
 */

#include "model/GroupChatDAO.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

GroupChatDAO::GroupChatDAO(Database& db) : db_(db) {}

bool GroupChatDAO::insert(GroupChat& group) {
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_chat (group_id, group_name, creator_id, avatar_path, announcement) "
             "VALUES ('%s', '%s', %lu, '%s', '%s')",
             group.group_id.c_str(), group.group_name.c_str(), 
             group.creator_id, group.avatar_path.c_str(), group.announcement.c_str());
    
    if (!db_.execute(sql)) {
        return false;
    }
    
    // 获取自增ID
    snprintf(sql, sizeof(sql), "SELECT LAST_INSERT_ID()");
    MYSQL_RES* res = db_.query(sql);
    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row && row[0]) {
            group.id = strtoull(row[0], nullptr, 10);
        }
        db_.freeResult(res);
    }
    
    return true;
}

bool GroupChatDAO::update(const GroupChat& group) {
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "UPDATE group_chat SET group_name='%s', avatar_path='%s', announcement='%s' "
             "WHERE group_id='%s'",
             group.group_name.c_str(), group.avatar_path.c_str(), 
             group.announcement.c_str(), group.group_id.c_str());
    
    return db_.execute(sql);
}

bool GroupChatDAO::remove(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM group_chat WHERE id=%lu", id);
    return db_.execute(sql);
}

GroupChat* GroupChatDAO::findById(uint64_t id) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, group_id, group_name, creator_id, avatar_path, announcement, create_time "
             "FROM group_chat WHERE id=%lu", id);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    GroupChat* group = new GroupChat();
    group->id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
    group->group_id = row[1] ? row[1] : "";
    group->group_name = row[2] ? row[2] : "";
    group->creator_id = row[3] ? strtoull(row[3], nullptr, 10) : 0;
    group->avatar_path = row[4] ? row[4] : "";
    group->announcement = row[5] ? row[5] : "";
    group->create_time = row[6] ? row[6] : "";
    
    db_.freeResult(res);
    return group;
}

GroupChat* GroupChatDAO::findByGroupId(const std::string& group_id) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, group_id, group_name, creator_id, avatar_path, announcement, create_time "
             "FROM group_chat WHERE group_id='%s'", group_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    GroupChat* group = new GroupChat();
    group->id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
    group->group_id = row[1] ? row[1] : "";
    group->group_name = row[2] ? row[2] : "";
    group->creator_id = row[3] ? strtoull(row[3], nullptr, 10) : 0;
    group->avatar_path = row[4] ? row[4] : "";
    group->announcement = row[5] ? row[5] : "";
    group->create_time = row[6] ? row[6] : "";
    
    db_.freeResult(res);
    return group;
}

std::vector<GroupChat> GroupChatDAO::findByCreator(uint64_t creator_id) {
    std::vector<GroupChat> groups;
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, group_id, group_name, creator_id, avatar_path, announcement, create_time "
             "FROM group_chat WHERE creator_id=%lu", creator_id);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return groups;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        GroupChat group;
        group.id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
        group.group_id = row[1] ? row[1] : "";
        group.group_name = row[2] ? row[2] : "";
        group.creator_id = row[3] ? strtoull(row[3], nullptr, 10) : 0;
        group.avatar_path = row[4] ? row[4] : "";
        group.announcement = row[5] ? row[5] : "";
        group.create_time = row[6] ? row[6] : "";
        groups.push_back(group);
    }
    
    db_.freeResult(res);
    return groups;
}

bool GroupChatDAO::addMember(const GroupMember& member) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "INSERT INTO group_member (group_id, user_id, role, nickname_in_group) "
             "VALUES (%lu, %lu, %d, '%s')",
             member.group_id, member.user_id, member.role, 
             member.nickname_in_group.c_str());
    
    return db_.execute(sql);
}

bool GroupChatDAO::removeMember(uint64_t group_id, uint64_t user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM group_member WHERE group_id=%lu AND user_id=%lu",
             group_id, user_id);
    
    return db_.execute(sql);
}

bool GroupChatDAO::updateMember(const GroupMember& member) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "UPDATE group_member SET role=%d, nickname_in_group='%s', mute_until='%s' "
             "WHERE group_id=%lu AND user_id=%lu",
             member.role, member.nickname_in_group.c_str(), 
             member.mute_until.c_str(), member.group_id, member.user_id);
    
    return db_.execute(sql);
}

std::vector<GroupMember> GroupChatDAO::getMembers(uint64_t group_id) {
    std::vector<GroupMember> members;
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, group_id, user_id, role, nickname_in_group, mute_until, join_time "
             "FROM group_member WHERE group_id=%lu", group_id);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return members;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        GroupMember member;
        member.id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
        member.group_id = row[1] ? strtoull(row[1], nullptr, 10) : 0;
        member.user_id = row[2] ? strtoull(row[2], nullptr, 10) : 0;
        member.role = row[3] ? atoi(row[3]) : 0;
        member.nickname_in_group = row[4] ? row[4] : "";
        member.mute_until = row[5] ? row[5] : "";
        member.join_time = row[6] ? row[6] : "";
        members.push_back(member);
    }
    
    db_.freeResult(res);
    return members;
}

GroupMember* GroupChatDAO::getMember(uint64_t group_id, uint64_t user_id) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, group_id, user_id, role, nickname_in_group, mute_until, join_time "
             "FROM group_member WHERE group_id=%lu AND user_id=%lu",
             group_id, user_id);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    GroupMember* member = new GroupMember();
    member->id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
    member->group_id = row[1] ? strtoull(row[1], nullptr, 10) : 0;
    member->user_id = row[2] ? strtoull(row[2], nullptr, 10) : 0;
    member->role = row[3] ? atoi(row[3]) : 0;
    member->nickname_in_group = row[4] ? row[4] : "";
    member->mute_until = row[5] ? row[5] : "";
    member->join_time = row[6] ? row[6] : "";
    
    db_.freeResult(res);
    return member;
}
