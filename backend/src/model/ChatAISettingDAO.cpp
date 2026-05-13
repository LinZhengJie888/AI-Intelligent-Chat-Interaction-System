/**
 * @file ChatAISettingDAO.cpp
 * @brief 聊天AI设置数据访问对象实现
 */

#include "model/ChatAISettingDAO.h"
#include <cstdio>
#include <cstring>

/**
 * @brief 构造函数
 */
ChatAISettingDAO::ChatAISettingDAO(Database& db) : db_(db) {}

/**
 * @brief 插入或更新聊天AI设置
 */
bool ChatAISettingDAO::insertOrUpdate(const ChatAISetting& setting) {
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "INSERT INTO chat_ai_settings (chat_key, nickname, tone, priority, updated_by) "
             "VALUES ('%s', '%s', %d, %d, '%s') "
             "ON DUPLICATE KEY UPDATE nickname='%s', tone=%d, priority=%d, updated_by='%s'",
             setting.chat_key.c_str(), setting.nickname.c_str(), 
             setting.tone, setting.priority, setting.updated_by.c_str(),
             setting.nickname.c_str(), setting.tone, setting.priority, setting.updated_by.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 根据聊天标识查询AI设置
 */
bool ChatAISettingDAO::findByChatKey(const std::string& chat_key, ChatAISetting& setting) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, chat_key, nickname, tone, priority, updated_by, update_time "
             "FROM chat_ai_settings WHERE chat_key='%s'",
             chat_key.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return false;
    }
    
    setting.id = row[0] ? strtoull(row[0], nullptr, 10) : 0;
    setting.chat_key = row[1] ? row[1] : "";
    setting.nickname = row[2] ? row[2] : "AI助手";
    setting.tone = row[3] ? atoi(row[3]) : 0;
    setting.priority = row[4] ? atoi(row[4]) : 1;
    setting.updated_by = row[5] ? row[5] : "";
    setting.update_time = row[6] ? row[6] : "";
    
    db_.freeResult(res);
    return true;
}

/**
 * @brief 更新聊天AI设置
 */
bool ChatAISettingDAO::update(const ChatAISetting& setting) {
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "UPDATE chat_ai_settings SET nickname='%s', tone=%d, priority=%d, updated_by='%s' "
             "WHERE chat_key='%s'",
             setting.nickname.c_str(), setting.tone, setting.priority, 
             setting.updated_by.c_str(), setting.chat_key.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 删除聊天AI设置
 */
bool ChatAISettingDAO::remove(const std::string& chat_key) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM chat_ai_settings WHERE chat_key='%s'",
             chat_key.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 检查聊天AI设置是否存在
 */
bool ChatAISettingDAO::exists(const std::string& chat_key) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "SELECT COUNT(*) FROM chat_ai_settings WHERE chat_key='%s'",
             chat_key.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    bool exists = (row && row[0] && atoi(row[0]) > 0);
    
    db_.freeResult(res);
    return exists;
}
