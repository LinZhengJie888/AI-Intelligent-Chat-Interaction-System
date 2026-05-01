/**
 * @file ChatRecordDAO.h
 * @brief 聊天记录数据访问对象
 * 
 * 提供聊天记录表的CRUD操作及消息状态更新功能。
 */

#pragma once

#include "model/ChatRecord.h"
#include "module/Database.h"
#include <vector>

/**
 * @class ChatRecordDAO
 * @brief 聊天记录数据访问对象
 */
class ChatRecordDAO {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    ChatRecordDAO(Database& db);
    
    /**
     * @brief 插入聊天记录
     * @param record 聊天记录对象（插入后会自动填充id）
     * @return 插入成功返回true，失败返回false
     */
    bool insert(ChatRecord& record);
    
    /**
     * @brief 更新聊天记录
     * @param record 聊天记录对象
     * @return 更新成功返回true，失败返回false
     */
    bool update(const ChatRecord& record);
    
    /**
     * @brief 删除聊天记录
     * @param id 聊天记录主键ID
     * @return 删除成功返回true，失败返回false
     */
    bool remove(uint64_t id);
    
    /**
     * @brief 根据主键ID查询聊天记录
     * @param id 聊天记录主键ID
     * @return 聊天记录对象指针，未找到返回nullptr（调用者负责释放内存）
     */
    ChatRecord* findById(uint64_t id);
    
    /**
     * @brief 查询两个用户之间的聊天记录
     * @param user1 用户1 ID
     * @param user2 用户2 ID
     * @param limit 返回条数限制，默认50
     * @return 聊天记录列表（按发送时间倒序）
     */
    std::vector<ChatRecord> findByUserPair(uint64_t user1, uint64_t user2, int limit = 50);
    
    /**
     * @brief 查询群聊聊天记录
     * @param group_id 群聊ID
     * @param limit 返回条数限制，默认50
     * @return 聊天记录列表（按发送时间倒序）
     */
    std::vector<ChatRecord> findByGroup(uint64_t group_id, int limit = 50);
    
    /**
     * @brief 标记消息为已读
     * @param id 聊天记录主键ID
     * @return 操作成功返回true，失败返回false
     */
    bool markAsRead(uint64_t id);
    
    /**
     * @brief 撤回消息
     * @param id 聊天记录主键ID
     * @return 操作成功返回true，失败返回false
     */
    bool recall(uint64_t id);
    
private:
    Database& db_; ///< 数据库连接引用
};
