/**
 * @file VerificationCode.h
 * @brief 验证码数据模型
 * 
 * 验证码表对应的数据结构，记录动态验证码信息，用于登录安全验证。
 */

#pragma once

#include <string>
#include <cstdint>

/**
 * @struct VerificationCode
 * @brief 验证码数据结构
 */
struct VerificationCode {
    uint64_t id;            ///< 主键ID，自增
    std::string phone;      ///< 手机号
    std::string code;       ///< 验证码（6位字符组合）
    std::string captcha_token; ///< 图像验证码关联token（用于前端刷新/校验）
    std::string expire_time;///< 过期时间
    std::string send_time;  ///< 发送时间
    int8_t is_used;         ///< 是否已使用（0-未使用，1-已使用）

    /**
     * @brief 构造函数，初始化默认值
     */
    VerificationCode() : id(0), is_used(0) {}
};
