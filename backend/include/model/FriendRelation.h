/**
 * @file FriendRelation.h
 * @brief 好友关系数据模型
 * 
 * 好友关系表对应的数据结构，记录用户之间的好友关系。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct FriendRelation
 * @brief 好友关系数据结构
 */
struct FriendRelation {
    uint64_t id;            ///< 主键ID，自增
    uint64_t user_id;       ///< 用户ID
    uint64_t friend_id;     ///< 好友用户ID
    std::string remark;     ///< 备注名
    std::string create_time;///< 创建时间

    /**
     * @brief 构造函数，初始化默认值
     */
    FriendRelation() : id(0), user_id(0), friend_id(0) {}
};
