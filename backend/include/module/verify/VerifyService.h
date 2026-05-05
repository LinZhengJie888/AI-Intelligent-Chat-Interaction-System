/**
 * @file VerifyService.h
 * @brief 验证码服务模块
 * 
 * 实现验证码的生成、校验、有效期管理等功能。
 * 支持图形验证码的生成和验证，用于登录注册安全验证。
 */

#pragma once

#include <string>
#include <map>
#include <mutex>
#include <ctime>

class Database;

/**
 * @struct CaptchaInfo
 * @brief 验证码信息结构体
 */
struct CaptchaInfo {
    std::string code;           ///< 验证码内容
    std::string phone;          ///< 关联手机号
    std::string token;          ///< 验证码token
    std::time_t expire_time;    ///< 过期时间
    std::time_t create_time;    ///< 创建时间
    bool used;                  ///< 是否已使用
    
    CaptchaInfo() : used(false) {}
};

/**
 * @class VerifyService
 * @brief 验证码服务类
 * 
 * 负责验证码的生成、存储、校验和过期管理。
 */
class VerifyService {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    VerifyService(Database& db);
    
    /**
     * @brief 析构函数
     */
    ~VerifyService();
    
    /**
     * @brief 初始化服务
     * @return 初始化成功返回true，失败返回false
     */
    bool init();
    
    /**
     * @brief 生成验证码
     * @param phone 手机号（可选）
     * @param token 输出的验证码token
     * @param captcha_image 输出的验证码图片（Base64编码）
     * @return 生成成功返回true，失败返回false
     */
    bool generateCaptcha(const std::string& phone, std::string& token, 
                        std::string& captcha_image);
    
    /**
     * @brief 验证验证码
     * @param token 验证码token
     * @param code 用户输入的验证码
     * @return 验证成功返回true，失败返回false
     */
    bool verifyCaptcha(const std::string& token, const std::string& code);
    
    /**
     * @brief 检查验证码是否有效
     * @param token 验证码token
     * @return 有效返回true，无效返回false
     */
    bool isCaptchaValid(const std::string& token);
    
    /**
     * @brief 刷新验证码
     * @param token 旧的验证码token
     * @param new_token 输出的新token
     * @param captcha_image 输出的新验证码图片
     * @return 刷新成功返回true，失败返回false
     */
    bool refreshCaptcha(const std::string& token, std::string& new_token, 
                       std::string& captcha_image);
    
    /**
     * @brief 清理过期验证码
     * @return 清理的验证码数量
     */
    int cleanExpiredCaptchas();

private:
    /**
     * @brief 生成随机验证码字符串
     * @param length 验证码长度
     * @return 验证码字符串
     */
    std::string generateRandomCode(int length = 6);
    
    /**
     * @brief 生成唯一token
     * @return token字符串
     */
    std::string generateToken();
    
    /**
     * @brief 生成验证码图片的Base64编码
     * @param code 验证码内容
     * @return Base64编码的图片字符串
     */
    std::string generateCaptchaImage(const std::string& code);
    
    /**
     * @brief 保存验证码到数据库
     * @param info 验证码信息
     * @return 保存成功返回true，失败返回false
     */
    bool saveCaptchaToDB(const CaptchaInfo& info);
    
    /**
     * @brief 从数据库加载验证码
     * @param token 验证码token
     * @param info 输出的验证码信息
     * @return 加载成功返回true，失败返回false
     */
    bool loadCaptchaFromDB(const std::string& token, CaptchaInfo& info);
    
    /**
     * @brief 更新验证码状态
     * @param token 验证码token
     * @param used 是否已使用
     * @return 更新成功返回true，失败返回false
     */
    bool updateCaptchaStatus(const std::string& token, bool used);

    Database& db_;                              ///< 数据库连接引用
    std::map<std::string, CaptchaInfo> captcha_cache_;  ///< 验证码缓存
    std::mutex cache_mutex_;                    ///< 缓存互斥锁
    
    static const int CAPTCHA_LENGTH = 6;        ///< 验证码长度
    static const int CAPTCHA_EXPIRE_SECONDS = 300;  ///< 验证码有效期（5分钟）
    static const int CAPTCHA_COOLDOWN_SECONDS = 60; ///< 验证码获取冷却时间（1分钟）
};
