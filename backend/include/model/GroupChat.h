/**
 * @file GroupChat.h
 * @brief 群聊数据模型
 * 
 * 群聊表对应的数据结构，包含群聊基本信息。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct GroupChat
 * @brief 群聊数据结构
 */
struct GroupChat {
    uint64_t id;                ///< 主键ID，自增
    std::string group_id;       ///< 群聊专属ID（唯一标识）
    std::string group_name;     ///< 群名（可修改）
    uint64_t creator_id;        ///< 创建者用户ID
    std::string avatar_path;    ///< 群聊头像存储路径
    std::string announcement;   ///< 群公告
    std::string create_time;    ///< 创建时间
    std::string update_time;    ///< 更新时间

    /**
     * @brief 构造函数，初始化默认值
     */
    GroupChat() : id(0), creator_id(0) {}
};
