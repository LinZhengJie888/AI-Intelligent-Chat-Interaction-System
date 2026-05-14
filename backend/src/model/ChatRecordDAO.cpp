/**
 * @file ChatRecordDAO.cpp
 * @brief 聊天记录数据访问对象实现
 */

#include "model/ChatRecordDAO.h"
#include <cstdio>
#include "module/redis/RedisClient.h"
#include <sstream>
#include <algorithm>

/**
 * @brief 构造函数
 * @param db 数据库连接引用
 */
ChatRecordDAO::ChatRecordDAO(Database& db) : db_(db) {}

/**
 * @brief 插入聊天记录
 * @param record 聊天记录对象（插入后会自动填充id）
 * @return 插入成功返回true，失败返回false
 */
bool ChatRecordDAO::insert(ChatRecord& record) {
    char sql[4096];
    
    // 转义content以防止SQL注入
    std::string escaped_content = db_.escapeString(record.content);
    
    // 处理NULL值的情况
    if (record.group_id > 0) {
        snprintf(sql, sizeof(sql),
                "INSERT INTO chat_record (sender_id, receiver_id, group_id, "
                "content, msg_type, is_ai, is_recalled, is_read) VALUES "
                "(%lu, %lu, %lu, '%s', %d, %d, %d, %d)",
                (unsigned long)record.sender_id,
                record.receiver_id > 0 ? (unsigned long)record.receiver_id : 0,
                (unsigned long)record.group_id,
                escaped_content.c_str(), record.msg_type,
                record.is_ai, record.is_recalled, record.is_read);
    } else {
        snprintf(sql, sizeof(sql),
                "INSERT INTO chat_record (sender_id, receiver_id, group_id, "
                "content, msg_type, is_ai, is_recalled, is_read) VALUES "
                "(%lu, %lu, NULL, '%s', %d, %d, %d, %d)",
                (unsigned long)record.sender_id,
                record.receiver_id > 0 ? (unsigned long)record.receiver_id : 0,
                escaped_content.c_str(), record.msg_type,
                record.is_ai, record.is_recalled, record.is_read);
    }
    
    if (!db_.execute(sql)) {
        return false;
    }
    
    // 获取自增ID
    record.id = mysql_insert_id(db_.getConnection());
    // 同步写入 Redis 列表缓存（用于快速获取最近消息）
    try {
        // 如果是私聊，使用两个用户的有序key；如果是群聊，使用群聊key
        std::string key;
        if (record.group_id > 0) {
            key = "chat:group:" + std::to_string(record.group_id);
        } else {
            uint64_t a = record.sender_id;
            uint64_t b = record.receiver_id;
            if (a == 0 || b == 0) {
                // 无效，跳过
            } else {
                if (a < b) key = "chat:pair:" + std::to_string(a) + ":" + std::to_string(b);
                else key = "chat:pair:" + std::to_string(b) + ":" + std::to_string(a);
            }
        }

        if (!key.empty() && RedisClient::getInstance().isConnected()) {
            // 以制表符分隔字段，便于解析（id\tsender\treceiver\tgroup\tcontent\tmsg_type\tis_ai\tis_recalled\tis_read\tsend_time）
            std::ostringstream oss;
            // sanitize content: replace tabs/newlines with spaces
            std::string content_s = record.content;
            std::replace(content_s.begin(), content_s.end(), '\t', ' ');
            std::replace(content_s.begin(), content_s.end(), '\n', ' ');
            std::replace(content_s.begin(), content_s.end(), '\r', ' ');
            oss << record.id << '\t' << record.sender_id << '\t' << record.receiver_id << '\t' << record.group_id
                << '\t' << content_s << '\t' << record.msg_type << '\t' << record.is_ai
                << '\t' << record.is_recalled << '\t' << record.is_read << '\t' << record.send_time;
            RedisClient::getInstance().lpush(key, oss.str());
        }
    } catch (...) {
        // ignore redis errors
    }
    return true;
}

/**
 * @brief 更新聊天记录
 * @param record 聊天记录对象
 * @return 更新成功返回true，失败返回false
 */
bool ChatRecordDAO::update(const ChatRecord& record) {
    char sql[4096];
    snprintf(sql, sizeof(sql),
            "UPDATE chat_record SET content='%s', msg_type=%d, is_ai=%d, "
            "is_recalled=%d, is_read=%d WHERE id=%lu",
            record.content.c_str(), record.msg_type, record.is_ai,
            record.is_recalled, record.is_read, (unsigned long)record.id);
    
    return db_.execute(sql);
}

/**
 * @brief 删除聊天记录
 * @param id 聊天记录主键ID
 * @return 删除成功返回true，失败返回false
 */
bool ChatRecordDAO::remove(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM chat_record WHERE id=%lu", (unsigned long)id);
    return db_.execute(sql);
}

/**
 * @brief 根据主键ID查询聊天记录
 * @param id 聊天记录主键ID
 * @return 聊天记录对象指针，未找到返回nullptr（调用者负责释放内存）
 */
ChatRecord* ChatRecordDAO::findById(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM chat_record WHERE id=%lu", (unsigned long)id);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    ChatRecord* record = new ChatRecord();
    record->id = strtoull(row[0], nullptr, 10);
    record->sender_id = strtoull(row[1], nullptr, 10);
    record->receiver_id = row[2] ? strtoull(row[2], nullptr, 10) : 0;
    record->group_id = row[3] ? strtoull(row[3], nullptr, 10) : 0;
    record->content = row[4] ? row[4] : "";
    record->msg_type = row[5] ? atoi(row[5]) : 0;
    record->is_ai = row[6] ? atoi(row[6]) : 0;
    record->is_recalled = row[7] ? atoi(row[7]) : 0;
    record->is_read = row[8] ? atoi(row[8]) : 0;
    record->send_time = row[9] ? row[9] : "";
    
    db_.freeResult(res);
    return record;
}

/**
 * @brief 查询两个用户之间的聊天记录
 * @param user1 用户1 ID
 * @param user2 用户2 ID
 * @param limit 返回条数限制，默认50
 * @return 聊天记录列表（按发送时间倒序）
 */
std::vector<ChatRecord> ChatRecordDAO::findByUserPair(uint64_t user1, uint64_t user2, int limit) {
    std::vector<ChatRecord> records;
    char sql[512];
    snprintf(sql, sizeof(sql),
            "SELECT * FROM chat_record WHERE "
            "((sender_id=%lu AND receiver_id=%lu) OR (sender_id=%lu AND receiver_id=%lu)) "
            "ORDER BY send_time DESC LIMIT %d",
            (unsigned long)user1, (unsigned long)user2,
            (unsigned long)user2, (unsigned long)user1, limit);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return records;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        ChatRecord record;
        record.id = strtoull(row[0], nullptr, 10);
        record.sender_id = strtoull(row[1], nullptr, 10);
        record.receiver_id = row[2] ? strtoull(row[2], nullptr, 10) : 0;
        record.group_id = row[3] ? strtoull(row[3], nullptr, 10) : 0;
        record.content = row[4] ? row[4] : "";
        record.msg_type = row[5] ? atoi(row[5]) : 0;
        record.is_ai = row[6] ? atoi(row[6]) : 0;
        record.is_recalled = row[7] ? atoi(row[7]) : 0;
        record.is_read = row[8] ? atoi(row[8]) : 0;
        record.send_time = row[9] ? row[9] : "";
        records.push_back(record);
    }
    
    db_.freeResult(res);
    return records;
}

/**
 * @brief 查询群聊聊天记录
 * @param group_id 群聊ID
 * @param limit 返回条数限制，默认50
 * @return 聊天记录列表（按发送时间倒序）
 */
std::vector<ChatRecord> ChatRecordDAO::findByGroup(uint64_t group_id, int limit) {
    std::vector<ChatRecord> records;
    char sql[512];
    snprintf(sql, sizeof(sql),
            "SELECT * FROM chat_record WHERE group_id=%lu "
            "ORDER BY send_time DESC LIMIT %d",
            (unsigned long)group_id, limit);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return records;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        ChatRecord record;
        record.id = strtoull(row[0], nullptr, 10);
        record.sender_id = strtoull(row[1], nullptr, 10);
        record.receiver_id = row[2] ? strtoull(row[2], nullptr, 10) : 0;
        record.group_id = row[3] ? strtoull(row[3], nullptr, 10) : 0;
        record.content = row[4] ? row[4] : "";
        record.msg_type = row[5] ? atoi(row[5]) : 0;
        record.is_ai = row[6] ? atoi(row[6]) : 0;
        record.is_recalled = row[7] ? atoi(row[7]) : 0;
        record.is_read = row[8] ? atoi(row[8]) : 0;
        record.send_time = row[9] ? row[9] : "";
        records.push_back(record);
    }
    
    db_.freeResult(res);
    return records;
}

/**
 * @brief 标记消息为已读
 * @param id 聊天记录主键ID
 * @return 操作成功返回true，失败返回false
 */
bool ChatRecordDAO::markAsRead(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE chat_record SET is_read=1 WHERE id=%lu", (unsigned long)id);
    return db_.execute(sql);
}

/**
 * @brief 撤回消息
 * @param id 聊天记录主键ID
 * @return 操作成功返回true，失败返回false
 */
bool ChatRecordDAO::recall(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE chat_record SET is_recalled=1 WHERE id=%lu", (unsigned long)id);
    return db_.execute(sql);
}
