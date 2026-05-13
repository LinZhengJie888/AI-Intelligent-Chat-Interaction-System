/**
 * @file ChatAISettingDAO.h
 * @brief 聊天AI设置数据访问对象
 * 
 * 提供聊天AI设置表的CRUD操作。
 * AI设置与聊天绑定，同一聊天中所有成员共享。
 */

#pragma once

#include "model/ChatAISetting.h"
#include "module/Database.h"
#include <string>

/**
 * @class ChatAISettingDAO
 * @brief 聊天AI设置数据访问对象
 */
class ChatAISettingDAO {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    ChatAISettingDAO(Database& db);
    
    /**
     * @brief 插入或更新聊天AI设置
     * @param setting 聊天AI设置对象
     * @return 操作成功返回true，失败返回false
     */
    bool insertOrUpdate(const ChatAISetting& setting);
    
    /**
     * @brief 根据聊天标识查询AI设置
     * @param chat_key 聊天标识
     * @param setting 输出的AI设置对象
     * @return 查询成功返回true，未找到返回false
     */
    bool findByChatKey(const std::string& chat_key, ChatAISetting& setting);
    
    /**
     * @brief 更新聊天AI设置
     * @param setting 聊天AI设置对象
     * @return 更新成功返回true，失败返回false
     */
    bool update(const ChatAISetting& setting);
    
    /**
     * @brief 删除聊天AI设置
     * @param chat_key 聊天标识
     * @return 删除成功返回true，失败返回false
     */
    bool remove(const std::string& chat_key);
    
    /**
     * @brief 检查聊天AI设置是否存在
     * @param chat_key 聊天标识
     * @return 存在返回true，否则返回false
     */
    bool exists(const std::string& chat_key);

private:
    Database& db_; ///< 数据库连接引用
};
