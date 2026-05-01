/**
 * @file GroupMessageRead.h
 * @brief 群消息已读数据模型
 * 
 * 群消息已读表对应的数据结构，记录群聊中每条消息的逐用户已读状态。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct GroupMessageRead
 * @brief 群消息已读数据结构
 */
struct GroupMessageRead {
    uint64_t id;            ///< 主键ID，自增
    uint64_t message_id;    ///< 引用chat_record.id
    uint64_t user_id;       ///< 已读用户ID（引用user.id）
    std::string read_time;  ///< 已读时间

    /**
     * @brief 构造函数，初始化默认值
     */
    GroupMessageRead() : id(0), message_id(0), user_id(0) {}
};
