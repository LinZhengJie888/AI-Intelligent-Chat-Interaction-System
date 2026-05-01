/**
 * @file GroupMember.h
 * @brief 群成员数据模型
 * 
 * 群成员表对应的数据结构，记录用户在群中的角色和状态。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct GroupMember
 * @brief 群成员数据结构
 */
struct GroupMember {
    uint64_t id;                    ///< 主键ID，自增
    uint64_t group_id;              ///< 群聊ID
    uint64_t user_id;               ///< 用户ID
    int8_t role;                    ///< 角色（0-普通成员，1-管理员，2-群主）
    std::string nickname_in_group;  ///< 群内昵称
    std::string mute_until;         ///< 禁言至（NULL表示未禁言）
    std::string join_time;          ///< 加入时间

    /**
     * @brief 构造函数，初始化默认值
     */
    GroupMember() : id(0), group_id(0), user_id(0), role(0) {}
};
