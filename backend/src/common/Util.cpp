/**
 * @file Util.cpp
 * @brief 公共工具函数实现
 */

#include "common/Util.h"
#include <openssl/md5.h>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cstring>

namespace util {

/**
 * @brief MD5加密函数
 * @param input 输入字符串
 * @return MD5加密后的32位十六进制字符串
 */
std::string md5(const std::string& input) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), digest);
    
    char md5string[33];
    for (int i = 0; i < 16; ++i) {
        sprintf(&md5string[i * 2], "%02x", (unsigned int)digest[i]);
    }
    return std::string(md5string);
}

/**
 * @brief 生成UUID（简化版，使用随机字符串）
 * @return 36位UUID字符串（小写）
 */
std::string generateUUID() {
    static const char hex[] = "0123456789abcdef";
    char uuid[37];
    
    srand(time(nullptr) ^ clock());
    
    // 格式: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid[i] = '-';
        } else {
            uuid[i] = hex[rand() % 16];
        }
    }
    uuid[36] = '\0';
    
    return std::string(uuid);
}

/**
 * @brief 生成随机验证码
 * @param length 验证码长度，默认6位
 * @return 包含数字和大小写字母的随机字符串
 */
std::string generateCaptcha(int length) {
    static const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::string result;
    
    srand(time(nullptr) ^ clock());
    for (int i = 0; i < length; ++i) {
        result += chars[rand() % (sizeof(chars) - 1)];
    }
    
    return result;
}

/**
 * @brief 获取当前时间字符串
 * @return "YYYY-MM-DD HH:MM:SS"格式的时间字符串
 */
std::string getCurrentTime() {
    time_t now = time(nullptr);
    return getTimeString(now);
}

/**
 * @brief 将time_t转换为时间字符串
 * @param t time_t类型时间
 * @return "YYYY-MM-DD HH:MM:SS"格式的时间字符串
 */
std::string getTimeString(time_t t) {
    struct tm* tm_info = localtime(&t);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buffer);
}

}
