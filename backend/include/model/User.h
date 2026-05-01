/**
 * @file User.h
 * @brief 用户数据模型
 * 
 * 用户表对应的数据结构，包含用户基本信息、AI助手配置等字段。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct User
 * @brief 用户数据结构
 */
struct User {
    uint64_t id;                ///< 主键ID，自增
    std::string user_id;        ///< 用户ID（唯一标识，不可修改）
    std::string username;       ///< 用户名（可重复，可修改）
    std::string nickname;       ///< 昵称
    std::string password;       ///< 加密密码（MD5）
    std::string phone;          ///< 手机号
    std::string avatar_path;    ///< 用户头像存储路径
    std::string ai_nickname;    ///< AI助手昵称
    int8_t ai_tone;             ///< AI语气（0-活泼，1-严谨，2-幽默）
    int8_t ai_priority;         ///< AI响应优先级（0-速度优先，1-质量优先）
    std::string create_time;    ///< 创建时间
    std::string update_time;    ///< 更新时间

    /**
     * @brief 构造函数，初始化默认值
     */
    User() : id(0), ai_tone(0), ai_priority(0) {}
};
