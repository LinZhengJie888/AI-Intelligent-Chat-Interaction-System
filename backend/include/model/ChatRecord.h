/**
 * @file ChatRecord.h
 * @brief 聊天记录数据模型
 * 
 * 聊天记录表对应的数据结构，记录好友聊天和群聊消息。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct ChatRecord
 * @brief 聊天记录数据结构
 */
struct ChatRecord {
    uint64_t id;            ///< 主键ID，自增
    uint64_t sender_id;     ///< 发送者用户ID
    uint64_t receiver_id;   ///< 接收者用户ID（一对一聊天时使用）
    uint64_t group_id;      ///< 群聊ID（群聊时使用）
    std::string content;    ///< 消息内容
    int8_t msg_type;        ///< 消息类型（0-文字，1-图片，2-表情）
    int8_t is_ai;           ///< 是否为AI回复（0-否，1-是）
    int8_t is_recalled;     ///< 是否已撤回（0-否，1-是）
    int8_t is_read;         ///< 是否已读（0-未读，1-已读）
    std::string send_time;  ///< 发送时间

    /**
     * @brief 构造函数，初始化默认值
     */
    ChatRecord() : id(0), sender_id(0), receiver_id(0), group_id(0),
                   msg_type(0), is_ai(0), is_recalled(0), is_read(0) {}
};
