/**
 * @file VerifyService.cpp
 * @brief 验证码服务模块实现
 */

#include "verify/VerifyService.h"
#include "module/Database.h"
#include "common/Util.h"
#include <iostream>
#include <sstream>
#include <random>
#include <algorithm>
#include <cstdio>

/**
 * @brief 构造函数
 */
VerifyService::VerifyService(Database& db) : db_(db) {}

/**
 * @brief 析构函数
 */
VerifyService::~VerifyService() {
    // 清理资源
}

/**
 * @brief 初始化服务
 */
bool VerifyService::init() {
    // 创建验证码表（如果不存在）
    std::string create_table_sql = 
        "CREATE TABLE IF NOT EXISTS verification_code ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "phone VARCHAR(20),"
        "code VARCHAR(10) NOT NULL,"
        "token VARCHAR(64) NOT NULL UNIQUE,"
        "expire_time DATETIME NOT NULL,"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "is_used TINYINT DEFAULT 0,"
        "INDEX idx_token (token),"
        "INDEX idx_phone (phone)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    if (!db_.execute(create_table_sql)) {
        std::cerr << "Failed to create verification_code table" << std::endl;
        return false;
    }
    
    std::cout << "VerifyService initialized successfully" << std::endl;
    return true;
}

/**
 * @brief 生成验证码
 */
bool VerifyService::generateCaptcha(const std::string& phone, std::string& token, 
                                    std::string& captcha_image) {
    // 检查冷却时间
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        for (const auto& pair : captcha_cache_) {
            if (pair.second.phone == phone && !pair.second.used) {
                std::time_t now = std::time(nullptr);
                if (now - pair.second.create_time < CAPTCHA_COOLDOWN_SECONDS) {
                    std::cerr << "Captcha cooldown for phone: " << phone << std::endl;
                    return false;
                }
            }
        }
    }
    
    // 生成验证码
    std::string code = generateRandomCode(CAPTCHA_LENGTH);
    token = generateToken();
    captcha_image = generateCaptchaImage(code);
    
    // 创建验证码信息
    CaptchaInfo info;
    info.code = code;
    info.phone = phone;
    info.token = token;
    info.create_time = std::time(nullptr);
    info.expire_time = info.create_time + CAPTCHA_EXPIRE_SECONDS;
    info.used = false;
    
    // 保存到缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        captcha_cache_[token] = info;
    }
    
    // 保存到数据库
    if (!saveCaptchaToDB(info)) {
        std::cerr << "Failed to save captcha to database" << std::endl;
        // 继续执行，因为缓存中已有数据
    }
    
    std::cout << "Captcha generated for phone: " << phone << std::endl;
    return true;
}

/**
 * @brief 验证验证码
 */
bool VerifyService::verifyCaptcha(const std::string& token, const std::string& code) {
    if (token.empty() || code.empty()) {
        return false;
    }
    
    CaptchaInfo info;
    bool found = false;
    
    // 先从缓存查找
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = captcha_cache_.find(token);
        if (it != captcha_cache_.end()) {
            info = it->second;
            found = true;
        }
    }
    
    // 缓存中未找到，从数据库查找
    if (!found) {
        if (!loadCaptchaFromDB(token, info)) {
            std::cerr << "Captcha not found: " << token << std::endl;
            return false;
        }
    }
    
    // 检查是否已使用
    if (info.used) {
        std::cerr << "Captcha already used: " << token << std::endl;
        return false;
    }
    
    // 检查是否过期
    std::time_t now = std::time(nullptr);
    if (now > info.expire_time) {
        std::cerr << "Captcha expired: " << token << std::endl;
        return false;
    }
    
    // 验证码比对（不区分大小写）
    std::string code_upper = code;
    std::string info_code_upper = info.code;
    std::transform(code_upper.begin(), code_upper.end(), code_upper.begin(), ::toupper);
    std::transform(info_code_upper.begin(), info_code_upper.end(), info_code_upper.begin(), ::toupper);
    
    if (code_upper != info_code_upper) {
        std::cerr << "Captcha mismatch: " << token << std::endl;
        return false;
    }
    
    // 标记为已使用
    updateCaptchaStatus(token, true);
    
    // 更新缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = captcha_cache_.find(token);
        if (it != captcha_cache_.end()) {
            it->second.used = true;
        }
    }
    
    std::cout << "Captcha verified successfully: " << token << std::endl;
    return true;
}

/**
 * @brief 检查验证码是否有效
 */
bool VerifyService::isCaptchaValid(const std::string& token) {
    if (token.empty()) {
        return false;
    }
    
    CaptchaInfo info;
    
    // 先从缓存查找
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = captcha_cache_.find(token);
        if (it != captcha_cache_.end()) {
            info = it->second;
            if (info.used) return false;
            std::time_t now = std::time(nullptr);
            return now <= info.expire_time;
        }
    }
    
    // 从数据库查找
    if (!loadCaptchaFromDB(token, info)) {
        return false;
    }
    
    if (info.used) return false;
    
    std::time_t now = std::time(nullptr);
    return now <= info.expire_time;
}

/**
 * @brief 刷新验证码
 */
bool VerifyService::refreshCaptcha(const std::string& token, std::string& new_token, 
                                   std::string& captcha_image) {
    // 查找原验证码信息
    CaptchaInfo info;
    bool found = false;
    
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = captcha_cache_.find(token);
        if (it != captcha_cache_.end()) {
            info = it->second;
            found = true;
        }
    }
    
    if (!found) {
        if (!loadCaptchaFromDB(token, info)) {
            return false;
        }
    }
    
    // 生成新验证码
    return generateCaptcha(info.phone, new_token, captcha_image);
}

/**
 * @brief 清理过期验证码
 */
int VerifyService::cleanExpiredCaptchas() {
    int count = 0;
    std::time_t now = std::time(nullptr);
    
    // 清理缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = captcha_cache_.begin();
        while (it != captcha_cache_.end()) {
            if (now > it->second.expire_time) {
                it = captcha_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    // 清理数据库
    char sql[256];
    snprintf(sql, sizeof(sql), 
             "DELETE FROM verification_code WHERE expire_time < NOW()");
    
    if (db_.execute(sql)) {
        std::cout << "Cleaned " << count << " expired captchas from cache" << std::endl;
    }
    
    return count;
}

/**
 * @brief 生成随机验证码字符串
 */
std::string VerifyService::generateRandomCode(int length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.size() - 1);
    
    std::string code;
    code.reserve(length);
    
    for (int i = 0; i < length; i++) {
        code += chars[dis(gen)];
    }
    
    return code;
}

/**
 * @brief 生成唯一token
 */
std::string VerifyService::generateToken() {
    return util::generateUUID();
}

/**
 * @brief 生成验证码图片的Base64编码
 * 
 * 这里简化实现，实际项目中应使用图像处理库生成验证码图片
 */
std::string VerifyService::generateCaptchaImage(const std::string& code) {
    // 简化实现：返回code的简单编码
    // 实际项目中应使用如Cairo、FreeType等库生成真正的验证码图片
    std::string svg = "<svg xmlns='http://www.w3.org/2000/svg' width='150' height='50'>"
                      "<rect width='100%' height='100%' fill='#f0f0f0'/>"
                      "<text x='10' y='35' font-family='Arial' font-size='24' fill='#333'>";
    svg += code;
    svg += "</text></svg>";
    
    // 简单的Base64编码（实际项目中应使用标准Base64库）
    // 这里直接返回SVG字符串，前端可以内联显示
    return svg;
}

/**
 * @brief 保存验证码到数据库
 */
bool VerifyService::saveCaptchaToDB(const CaptchaInfo& info) {
    char sql[1024];
    char expire_time_str[64];
    
    // 格式化过期时间
    std::strftime(expire_time_str, sizeof(expire_time_str), 
                  "%Y-%m-%d %H:%M:%S", std::localtime(&info.expire_time));
    
    snprintf(sql, sizeof(sql),
             "INSERT INTO verification_code (phone, code, token, expire_time, is_used) "
             "VALUES ('%s', '%s', '%s', '%s', %d)",
             info.phone.c_str(), info.code.c_str(), info.token.c_str(),
             expire_time_str, info.used ? 1 : 0);
    
    return db_.execute(sql);
}

/**
 * @brief 从数据库加载验证码
 */
bool VerifyService::loadCaptchaFromDB(const std::string& token, CaptchaInfo& info) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT id, phone, code, token, expire_time, create_time, is_used "
             "FROM verification_code WHERE token='%s'",
             token.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return false;
    }
    
    info.phone = row[1] ? row[1] : "";
    info.code = row[2] ? row[2] : "";
    info.token = row[3] ? row[3] : "";
    info.used = (row[6] && std::string(row[6]) == "1");
    
    // 解析过期时间
    if (row[4]) {
        struct tm tm_time;
        strptime(row[4], "%Y-%m-%d %H:%M:%S", &tm_time);
        info.expire_time = mktime(&tm_time);
    }
    
    // 解析创建时间
    if (row[5]) {
        struct tm tm_time;
        strptime(row[5], "%Y-%m-%d %H:%M:%S", &tm_time);
        info.create_time = mktime(&tm_time);
    }
    
    db_.freeResult(res);
    return true;
}

/**
 * @brief 更新验证码状态
 */
bool VerifyService::updateCaptchaStatus(const std::string& token, bool used) {
    char sql[256];
    snprintf(sql, sizeof(sql),
             "UPDATE verification_code SET is_used=%d WHERE token='%s'",
             used ? 1 : 0, token.c_str());
    
    return db_.execute(sql);
}
