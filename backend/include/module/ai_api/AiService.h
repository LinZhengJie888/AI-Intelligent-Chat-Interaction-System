/**
 * @file AiService.h
 * @brief AI服务模块
 * 
 * 实现第三方AI API调用、回复拆分、异常重试等功能。
 * 支持按键和@召唤两种方式调用AI。
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

class Database;

/**
 * @enum AITone
 * @brief AI语气枚举
 */
enum class AITone {
    DEFAULT = 0,    ///< 默认
    FORMAL = 1,     ///< 正式
    LIVELY = 2      ///< 活泼
};

/**
 * @enum AIPriority
 * @brief AI响应优先级枚举
 */
enum class AIPriority {
    BALANCED = 0,   ///< 平衡
    SPEED = 1,      ///< 速度优先
    QUALITY = 2     ///< 质量优先
};

/**
 * @struct AIRequest
 * @brief AI请求结构体
 */
struct AIRequest {
    std::string request_id;     ///< 请求ID
    std::string user_id;        ///< 用户ID
    std::string target_id;      ///< 目标ID（用户ID或群聊ID）
    std::string question;       ///< 问题内容
    bool is_group;              ///< 是否是群聊
    std::string extra;          ///< 额外数据（包含chatKey等）
    std::string timestamp;      ///< 时间戳
};

/**
 * @struct AIResponse
 * @brief AI响应结构体
 */
struct AIResponse {
    std::string request_id;     ///< 请求ID
    std::vector<std::string> messages;  ///< 拆分后的消息列表
    bool success;               ///< 是否成功
    std::string error_msg;      ///< 错误信息
};

/**
 * @struct AIConfig
 * @brief AI配置结构体
 */
struct AIServiceConfig {
    std::string api_url;        ///< API地址
    std::string api_key;        ///< API密钥
    std::string model;          ///< 模型名称
    int timeout_seconds;        ///< 超时时间（秒）
    int connect_timeout;        ///< 连接超时时间（秒）
    int max_retries;            ///< 最大重试次数
    int max_message_length;     ///< 单条消息最大长度
    bool enable_cache;          ///< 是否启用缓存
};

/**
 * @class AiService
 * @brief AI服务类
 * 
 * 负责处理AI相关的业务逻辑，包括API调用、回复拆分、缓存管理等。
 */
class AiService {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    AiService(Database& db);
    
    /**
     * @brief 析构函数
     */
    ~AiService();
    
    /**
     * @brief 初始化服务
     * @return 初始化成功返回true，失败返回false
     */
    bool init();
    
    /**
     * @brief 处理AI请求
     * @param user_id 用户ID
     * @param target_id 目标ID（用户ID或群聊ID）
     * @param question 问题内容
     * @param is_group 是否是群聊
     * @param extra 额外数据（包含chatKey等）
     * @return 请求ID
     */
    std::string processRequest(const std::string& user_id, const std::string& target_id, 
                               const std::string& question, bool is_group,
                               const std::string& extra = "");
    
    /**
     * @brief 同步调用AI API
     * @param question 问题内容
     * @param tone AI语气
     * @param priority AI优先级
     * @param response 输出的响应内容
     * @return 调用成功返回true，失败返回false
     */
    bool callAIAPI(const std::string& question, AITone tone, AIPriority priority, 
                  std::string& response);
    
    /**
     * @brief 拆分AI回复
     * @param response 原始回复内容
     * @param max_length 单条消息最大长度
     * @return 拆分后的消息列表
     */
    std::vector<std::string> splitResponse(const std::string& response, int max_length = 50);
    
    /**
     * @brief 设置AI配置
     * @param config AI配置
     */
    void setConfig(const AIServiceConfig& config);
    
    /**
     * @brief 获取AI配置
     * @return AI配置
     */
    const AIServiceConfig& getConfig() const;
    
    /**
     * @brief 更新聊天AI设置
     * @param chat_key 聊天标识（single:{userId} / group:{groupId} / ai:ai）
     * @param nickname AI昵称
     * @param tone AI语气
     * @param priority AI优先级
     * @param updated_by 修改者用户ID
     * @return 更新成功返回true，失败返回false
     */
    bool updateChatAISettings(const std::string& chat_key, const std::string& nickname, 
                              int tone, int priority, const std::string& updated_by);
    
    /**
     * @brief 获取聊天AI设置
     * @param chat_key 聊天标识（single:{userId} / group:{groupId} / ai:ai）
     * @param nickname 输出的AI昵称
     * @param tone 输出的AI语气
     * @param priority 输出的AI优先级
     * @return 获取成功返回true，失败返回false
     */
    bool getChatAISettings(const std::string& chat_key, std::string& nickname, 
                           int& tone, int& priority);
    
    /**
     * @brief 清理过期缓存
     * @return 清理的缓存数量
     */
    int cleanExpiredCache();
    
    /**
     * @brief 获取缓存的AI回复
     * @param question 问题内容
     * @param response 输出的回复内容
     * @return 命中缓存返回true，否则返回false
     */
    bool getCachedResponse(const std::string& question, std::string& response);
    
    /**
     * @brief 缓存AI回复
     * @param question 问题内容
     * @param response 回复内容
     * @return 缓存成功返回true，失败返回false
     */
    bool cacheResponse(const std::string& question, const std::string& response);

private:
    /**
     * @brief 创建AI缓存表
     * @return 创建成功返回true，失败返回false
     */
    bool createAICacheTable();
    
    /**
     * @brief 创建AI日志表
     * @return 创建成功返回true，失败返回false
     */
    bool createAILogTable();
    
    /**
     * @brief 生成请求ID
     * @return 请求ID字符串
     */
    std::string generateRequestId();
    
    /**
     * @brief 记录AI调用日志
     * @param request_id 请求ID
     * @param user_id 用户ID
     * @param question 问题内容
     * @param response 回复内容
     * @param success 是否成功
     * @param error_msg 错误信息
     * @param response_time 响应时间（毫秒）
     */
    void logAICall(const std::string& request_id, const std::string& user_id, 
                  const std::string& question, const std::string& response, 
                  bool success, const std::string& error_msg, int response_time);
    
    /**
     * @brief 发送AI回复给用户
     * @param user_id 用户ID
     * @param target_id 目标ID
     * @param messages 消息列表
     * @param is_group 是否是群聊
     */
    void sendAIResponse(const std::string& user_id, const std::string& target_id, 
                       const std::vector<std::string>& messages, bool is_group,
                       const std::string& origin_user_id);
    
    /**
     * @brief 异步处理AI请求
     * @param request AI请求
     */
    void asyncProcessRequest(const AIRequest& request);
    
    /**
     * @brief 调用HTTP请求
     * @param url 请求URL
     * @param headers 请求头
     * @param body 请求体
     * @param response 输出的响应
     * @param timeout 超时时间
     * @return 调用成功返回true，失败返回false
     */
    bool httpPost(const std::string& url, const std::map<std::string, std::string>& headers, 
                 const std::string& body, std::string& response, int timeout);
    
    /**
     * @brief 解析AI API响应
     * @param response 原始响应
     * @param content 输出的内容
     * @return 解析成功返回true，失败返回false
     */
    bool parseAIResponse(const std::string& response, std::string& content);

    Database& db_;                              ///< 数据库连接引用
    AIServiceConfig config_;                    ///< AI配置
    
    // 缓存
    std::map<std::string, std::pair<std::string, std::time_t>> response_cache_;
    std::mutex cache_mutex_;
    
    // 请求队列
    std::map<std::string, AIRequest> pending_requests_;
    std::mutex request_mutex_;
    
    // 统计信息
    std::atomic<int> total_requests_;
    std::atomic<int> success_requests_;
    std::atomic<int> failed_requests_;
    
    static const int DEFAULT_TIMEOUT = 30;          ///< 默认超时时间（秒）
    static const int DEFAULT_CONNECT_TIMEOUT = 10;  ///< 默认连接超时时间（秒）
    static const int DEFAULT_MAX_RETRIES = 3;       ///< 默认最大重试次数
    static const int DEFAULT_MAX_MESSAGE_LENGTH = 50;   ///< 默认单条消息最大长度
    static const int CACHE_EXPIRE_HOURS = 24;       ///< 缓存过期时间（小时）
};
