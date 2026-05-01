/**
 * @file GroupRequest.h
 * @brief 加群请求数据模型
 * 
 * 加群请求表对应的数据结构，记录用户加入群聊的申请信息。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct GroupRequest
 * @brief 加群请求数据结构
 */
struct GroupRequest {
    uint64_t id;                ///< 主键ID，自增
    uint64_t group_id;          ///< 群聊ID
    uint64_t from_user_id;      ///< 申请人用户ID
    std::string request_msg;    ///< 申请消息
    int8_t status;              ///< 状态（0-待处理，1-同意，2-拒绝）
    std::string create_time;    ///< 创建时间
    std::string update_time;    ///< 更新时间

    /**
     * @brief 构造函数，初始化默认值
     */
    GroupRequest() : id(0), group_id(0), from_user_id(0), status(0) {}
};
