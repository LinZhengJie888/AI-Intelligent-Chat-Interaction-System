/**
 * @file FriendRequest.h
 * @brief 好友请求数据模型
 * 
 * 好友请求表对应的数据结构，记录好友添加请求信息及状态。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct FriendRequest
 * @brief 好友请求数据结构
 */
struct FriendRequest {
    uint64_t id;                ///< 主键ID，自增
    uint64_t from_user_id;      ///< 请求方用户ID
    uint64_t to_user_id;        ///< 接收方用户ID
    std::string request_msg;    ///< 验证消息
    int8_t status;              ///< 状态（0-待处理，1-同意，2-拒绝）
    std::string cooling_time;   ///< 冷却时间（拒绝后下次可请求时间）
    std::string create_time;    ///< 创建时间
    std::string update_time;    ///< 更新时间

    /**
     * @brief 构造函数，初始化默认值
     */
    FriendRequest() : id(0), from_user_id(0), to_user_id(0), status(0) {}
};
