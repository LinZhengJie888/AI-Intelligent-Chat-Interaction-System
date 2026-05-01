/**
 * @file Util.h
 * @brief 公共工具函数声明
 * 
 * 提供常用的工具函数，包括加密、UUID生成、验证码生成、时间格式化等。
 */

#pragma once

#include <string>
#include <cstdint>
#include <ctime>

namespace util {

/**
 * @brief MD5加密函数
 * @param input 输入字符串
 * @return MD5加密后的32位十六进制字符串
 */
std::string md5(const std::string& input);

/**
 * @brief 生成UUID
 * @return 36位UUID字符串（小写）
 */
std::string generateUUID();

/**
 * @brief 生成随机验证码
 * @param length 验证码长度，默认6位
 * @return 包含数字和大小写字母的随机字符串
 */
std::string generateCaptcha(int length = 6);

/**
 * @brief 获取当前时间字符串
 * @return "YYYY-MM-DD HH:MM:SS"格式的时间字符串
 */
std::string getCurrentTime();

/**
 * @brief 将time_t转换为时间字符串
 * @param t time_t类型时间
 * @return "YYYY-MM-DD HH:MM:SS"格式的时间字符串
 */
std::string getTimeString(time_t t);

}
