/**
 * @file AiService.cpp
 * @brief AI服务模块实现
 */

#include "ai_api/AiService.h"
#include "module/Database.h"
#include "module/Config.h"
#include "module/ChatService.h"
#include "module/redis/RedisClient.h"
#include "model/ChatRecord.h"
#include "model/ChatRecordDAO.h"
#include "model/ChatAISetting.h"
#include "model/ChatAISettingDAO.h"
#include "common/Util.h"
#include "common/JsonUtil.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>
#include <regex>
#include <unistd.h>
#include <fcntl.h>

// 简化的HTTP客户端实现（实际项目中应使用libcurl等库）
#ifdef USE_CURL
#include <curl/curl.h>
#endif

/**
 * @brief JSON转义辅助函数
 */
static std::string escapeJson(const std::string& str) {
    return JsonUtil::escapeString(str);
}

/**
 * @brief 根据用户ID字符串获取数字ID
 * @param db 数据库连接
 * @param user_id 用户ID字符串
 * @return 用户数字ID，失败返回0
 */
static uint64_t getUserIdNum(Database& db, const std::string& user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT id FROM user WHERE user_id='%s'", user_id.c_str());
    
    MYSQL_RES* res = db.query(sql);
    if (!res) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        db.freeResult(res);
        return 0;
    }
    
    uint64_t id = strtoull(row[0], nullptr, 10);
    db.freeResult(res);
    return id;
}

/**
 * @brief 根据群聊ID字符串获取数字ID
 * @param db 数据库连接
 * @param group_id 群聊ID字符串
 * @return 群聊数字ID，失败返回0
 */
static uint64_t getGroupNumId(Database& db, const std::string& group_id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT id FROM group_chat WHERE group_id='%s'", group_id.c_str());
    
    MYSQL_RES* res = db.query(sql);
    if (!res) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        db.freeResult(res);
        return 0;
    }
    
    uint64_t id = strtoull(row[0], nullptr, 10);
    db.freeResult(res);
    return id;
}

/**
 * @brief 获取JSON值辅助函数
 */
static std::string getJsonValue(const std::string& json, const std::string& key) {
    return JsonUtil::getString(json, key);
}

#ifdef USE_CURL
/**
 * @brief CURL写回调函数
 */
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}
#endif

/**
 * @brief 构造函数
 */
AiService::AiService(Database& db) 
    : db_(db), stop_workers_(false), total_requests_(0), success_requests_(0), failed_requests_(0) {
    // 初始化默认配置
    config_.api_url = "";
    config_.api_key = "";
    config_.model = "gpt-3.5-turbo";
    config_.timeout_seconds = DEFAULT_TIMEOUT;
    config_.connect_timeout = DEFAULT_CONNECT_TIMEOUT;
    config_.max_retries = DEFAULT_MAX_RETRIES;
    config_.max_message_length = DEFAULT_MAX_MESSAGE_LENGTH;
    config_.enable_cache = true;
    config_.enable_stream = true;
    config_.context_message_count = 10;
    config_.max_context_tokens = 2000;
    
    // 启动工作线程
    for (int i = 0; i < WORKER_COUNT; i++) {
        worker_threads_.emplace_back([this]() {
            while (true) {
                AIRequest request;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex_);
                    queue_cv_.wait(lock, [this]() { 
                        return stop_workers_ || !task_queue_.empty(); 
                    });
                    
                    if (stop_workers_ && task_queue_.empty()) {
                        return;
                    }
                    
                    request = task_queue_.front();
                    task_queue_.pop();
                }
                
                // 处理请求
                asyncProcessRequest(request);
            }
        });
    }
}

/**
 * @brief 析构函数
 */
AiService::~AiService() {
    // 停止工作线程
    stop_workers_ = true;
    queue_cv_.notify_all();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

/**
 * @brief 初始化服务
 */
bool AiService::init() {
    // 创建AI缓存表
    if (!createAICacheTable()) {
        std::cerr << "Failed to create AI cache table" << std::endl;
        return false;
    }
    
    // 创建AI日志表
    if (!createAILogTable()) {
        std::cerr << "Failed to create AI log table" << std::endl;
        return false;
    }
    
    // 创建聊天AI设置表
    if (!createChatAISettingsTable()) {
        std::cerr << "Failed to create chat AI settings table" << std::endl;
        return false;
    }
    
    // 从配置文件加载AI配置
    Config& config = Config::getInstance();
    const AIConfig& ai_config = config.getAIConfig();
    
    config_.api_url = ai_config.api_url;
    config_.api_key = ai_config.api_key;
    config_.model = ai_config.model;
    config_.enable_stream = ai_config.enable_stream;
    config_.context_message_count = ai_config.context_message_count;
    config_.max_context_tokens = ai_config.max_context_tokens;
    
    // 确保API URL以/chat/completions结尾
    if (!config_.api_url.empty()) {
        // 移除末尾的斜杠
        while (!config_.api_url.empty() && config_.api_url.back() == '/') {
            config_.api_url.pop_back();
        }
        // 添加/chat/completions路径
        if (config_.api_url.find("/chat/completions") == std::string::npos) {
            config_.api_url += "/chat/completions";
        }
    }
    
    std::cout << "AiService initialized successfully" << std::endl;
    std::cout << "AI API URL: " << (config_.api_url.empty() ? "Not configured" : config_.api_url) << std::endl;
    std::cout << "AI Model: " << config_.model << std::endl;
    std::cout << "AI Stream: " << (config_.enable_stream ? "enabled" : "disabled") << std::endl;
    std::cout << "AI Context messages: " << config_.context_message_count << std::endl;
    std::cout << "AI Max context tokens: " << config_.max_context_tokens << std::endl;

    // 初始化 Redis 客户端（用于 AI 回复缓存等）
    Config& conf = Config::getInstance();
    const auto& redis_conf = conf.getRedisConfig();
    RedisClient::getInstance().init(redis_conf.host, redis_conf.port);
    
    return true;
}

/**
 * @brief 处理AI请求
 */
std::string AiService::processRequest(const std::string& user_id, const std::string& target_id, 
                                       const std::string& question, bool is_group,
                                       const std::string& extra) {
    std::string request_id = generateRequestId();
    
    // 创建请求对象
    AIRequest request;
    request.request_id = request_id;
    request.user_id = user_id;
    request.target_id = target_id;
    request.question = question;
    request.is_group = is_group;
    request.extra = extra;
    request.timestamp = util::getCurrentTime();
    
    // 保存到待处理队列
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        pending_requests_[request_id] = request;
    }
    
    // 将请求添加到任务队列
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        task_queue_.push(request);
    }
    queue_cv_.notify_one();
    
    total_requests_++;
    
    std::cout << "AI request queued: " << request_id << " from " << user_id << std::endl;
    return request_id;
}

/**
 * @brief 同步调用AI API
 */
bool AiService::callAIAPI(const std::string& question, AITone tone, AIPriority priority, 
                           std::string& response, const std::vector<ContextMessage>& context) {
    if (config_.api_url.empty() || config_.api_key.empty()) {
        std::cerr << "[AI] API not configured" << std::endl;
        response = "AI服务未配置，请联系管理员";
        return false;
    }
    
    std::cout << "[AI] Calling API: " << config_.api_url << std::endl;
    std::cout << "[AI] Model: " << config_.model << std::endl;
    
    // 检查缓存（仅当无上下文时使用question-only缓存）
    if (config_.enable_cache && context.empty()) {
        std::string cached_response;
        if (getCachedResponse(question, cached_response)) {
            response = cached_response;
            return true;
        }
    }
    
    // 构建请求体
    std::string tone_prompt;
    switch (tone) {
        case AITone::FORMAL:
            tone_prompt = "请用正式、专业的语气回答。";
            break;
        case AITone::LIVELY:
            tone_prompt = "请用活泼、轻松的语气回答。";
            break;
        default:
            tone_prompt = "请用简洁、友好的语气回答。";
            break;
    }
    
    std::string priority_prompt;
    switch (priority) {
        case AIPriority::SPEED:
            priority_prompt = "请简洁回答，控制在50字以内。";
            break;
        case AIPriority::QUALITY:
            priority_prompt = "请详细、准确地回答。";
            break;
        default:
            priority_prompt = "请平衡简洁与详细地回答。";
            break;
    }
    
    std::string system_prompt = "你是一个智能聊天助手。" + tone_prompt + priority_prompt + 
                                "回答要简洁、精准，避免冗余。如果需要分点说明，请用数字编号。";
    
    // 构建JSON请求体（含上下文历史）
    std::ostringstream body;
    body << "{"
         << "\"model\":\"" << config_.model << "\","
         << "\"messages\":["
         << "{\"role\":\"system\",\"content\":\"" << escapeJson(system_prompt) << "\"}";
    
    // 添加上下文历史消息
    for (const auto& ctx : context) {
        body << ",{\"role\":\"" << ctx.role << "\",\"content\":\"" << escapeJson(ctx.content) << "\"}";
    }
    
    body << ",{\"role\":\"user\",\"content\":\"" << escapeJson(question) << "\"}"
         << "],"
         << "\"temperature\":0.7,"
         << "\"max_tokens\":500"
         << "}";
    
    // 设置请求头
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + config_.api_key;
    
    // 调用API（带重试）
    std::string api_response;
    bool success = false;
    
    std::cout << "[AI] Request body: " << body.str().substr(0, 200) << "..." << std::endl;
    
    for (int retry = 0; retry < config_.max_retries; retry++) {
        std::cout << "[AI] HTTP request attempt " << (retry + 1) << "/" << config_.max_retries << std::endl;
        if (httpPost(config_.api_url, headers, body.str(), api_response, config_.timeout_seconds)) {
            std::cout << "[AI] HTTP response received, length=" << api_response.length() << std::endl;
            // 解析响应
            std::string content;
            std::cout << "[AI] parseAIResponse returned, checking result..." << std::endl;
            if (parseAIResponse(api_response, content)) {
                std::cout << "[AI] parseAIResponse returned true" << std::endl;
                response = content;
                success = true;
                std::cout << "[AI] Parsed content: " << content.substr(0, 200) << std::endl;
                
                // 缓存响应
                if (config_.enable_cache) {
                    std::cout << "[AI] Caching response..." << std::endl;
                    cacheResponse(question, response);
                    std::cout << "[AI] Response cached" << std::endl;
                }
                
                std::cout << "[AI] Breaking from retry loop..." << std::endl;
                break;
            } else {
                std::cerr << "[AI] Failed to parse response" << std::endl;
            }
        } else {
            std::cerr << "[AI] HTTP request failed" << std::endl;
        }
        
        std::cerr << "[AI] API call failed, retry " << (retry + 1) << "/" << config_.max_retries << std::endl;
        
        // 重试前等待
        if (retry < config_.max_retries - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (retry + 1)));
        }
    }
    
    std::cout << "[AI] callAIAPI returning: " << success << std::endl;
    
    if (!success) {
        response = "AI服务暂时不可用，请稍后再试";
        failed_requests_++;
    } else {
        success_requests_++;
    }
    
    return success;
}

/**
 * @brief 流式调用AI API
 */
bool AiService::callAIAPIStream(const std::string& question, 
                                 const std::vector<ContextMessage>& context,
                                 AITone tone, AIPriority priority, 
                                 const AIStreamCallback& callback) {
    if (config_.api_url.empty() || config_.api_key.empty()) {
        std::cerr << "[AI] API not configured" << std::endl;
        callback("AI服务未配置，请联系管理员", true);
        return false;
    }
    
    std::cout << "[AI] Calling streaming API: " << config_.api_url << std::endl;
    std::cout << "[AI] Model: " << config_.model << ", context size=" << context.size() << std::endl;
    
    // 构建请求体（含上下文 + stream=true）
    std::string tone_prompt;
    switch (tone) {
        case AITone::FORMAL:
            tone_prompt = "请用正式、专业的语气回答。";
            break;
        case AITone::LIVELY:
            tone_prompt = "请用活泼、轻松的语气回答。";
            break;
        default:
            tone_prompt = "请用简洁、友好的语气回答。";
            break;
    }
    
    std::string priority_prompt;
    switch (priority) {
        case AIPriority::SPEED:
            priority_prompt = "请简洁回答，控制在50字以内。";
            break;
        case AIPriority::QUALITY:
            priority_prompt = "请详细、准确地回答。";
            break;
        default:
            priority_prompt = "请平衡简洁与详细地回答。";
            break;
    }
    
    std::string system_prompt = "你是一个智能聊天助手。" + tone_prompt + priority_prompt + 
                                "回答要简洁、精准，避免冗余。如果需要分点说明，请用数字编号。";
    
    std::ostringstream body;
    body << "{"
         << "\"model\":\"" << config_.model << "\","
         << "\"stream\":true,"
         << "\"messages\":["
         << "{\"role\":\"system\",\"content\":\"" << escapeJson(system_prompt) << "\"}";
    
    for (const auto& ctx : context) {
        body << ",{\"role\":\"" << ctx.role << "\",\"content\":\"" << escapeJson(ctx.content) << "\"}";
    }
    
    body << ",{\"role\":\"user\",\"content\":\"" << escapeJson(question) << "\"}"
         << "],"
         << "\"temperature\":0.7,"
         << "\"max_tokens\":500"
         << "}";
    
    // 设置请求头
    std::map<std::string, std::string> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + config_.api_key;
    headers["Accept"] = "text/event-stream";
    
    // 流式HTTP请求（回调中实时推送）
    bool ok = httpPostStream(config_.api_url, headers, body.str(), callback, config_.timeout_seconds);
    if (!ok) {
        std::cerr << "[AI] Streaming request failed" << std::endl;
    }
    return ok;
}

/**
 * @brief 拆分AI回复（改进版：按UTF-8字符计数，自然语句边界切分）
 */
std::vector<std::string> AiService::splitResponse(const std::string& response, int max_length) {
    std::vector<std::string> messages;
    
    if (response.empty()) {
        return messages;
    }
    
    // 辅助函数：计算UTF-8字符数（非字节数）
    auto utf8Length = [](const std::string& str) -> size_t {
        size_t count = 0;
        for (size_t i = 0; i < str.size();) {
            unsigned char c = str[i];
            if (c < 0x80) i += 1;           // ASCII
            else if ((c & 0xE0) == 0xC0) i += 2;  // 2字节
            else if ((c & 0xF0) == 0xE0) i += 3;  // 3字节（中文）
            else if ((c & 0xF8) == 0xF0) i += 4;  // 4字节
            else i += 1;
            count++;
        }
        return count;
    };
    
    // 如果回复字符数小于等于最大长度，直接返回
    if (utf8Length(response) <= static_cast<size_t>(max_length)) {
        messages.push_back(response);
        return messages;
    }
    
    // 中文标点符号（UTF-8编码，各3字节）
    const std::string period = "。";
    const std::string comma = "，";
    const std::string exclamation = "！";
    const std::string question_mark = "？";
    const std::string newline = "\n";
    
    // 按句子拆分
    std::string current_message;
    std::string remaining = response;
    
    while (!remaining.empty()) {
        std::string sentence;
        size_t pos = std::string::npos;
        size_t sep_len = 3;  // 分隔符长度（UTF-8中文标点3字节）
        
        // 查找最近的句号、感叹号、问号、换行
        size_t period_pos = remaining.find(period);
        size_t exclamation_pos = remaining.find(exclamation);
        size_t question_pos = remaining.find(question_mark);
        size_t newline_pos = remaining.find(newline);
        
        // 找到最早出现的分隔符
        pos = period_pos;
        if (pos == std::string::npos || (exclamation_pos != std::string::npos && exclamation_pos < pos)) {
            pos = exclamation_pos;
        }
        if (pos == std::string::npos || (question_pos != std::string::npos && question_pos < pos)) {
            pos = question_pos;
        }
        if (pos == std::string::npos || (newline_pos != std::string::npos && newline_pos < pos)) {
            pos = newline_pos;
            sep_len = 1;  // 换行符1字节
        }
        
        if (pos == std::string::npos) {
            sentence = remaining;
            remaining.clear();
        } else {
            sentence = remaining.substr(0, pos + sep_len);
            remaining = remaining.substr(pos + sep_len);
        }
        
        // 去除前导空格
        size_t start = sentence.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            sentence = sentence.substr(start);
        }
        
        if (sentence.empty()) continue;
        
        // 按UTF-8字符数判断是否超过最大长度
        if (utf8Length(current_message) + utf8Length(sentence) > static_cast<size_t>(max_length)) {
            if (!current_message.empty()) {
                messages.push_back(current_message);
                current_message.clear();
            }
            
            // 如果单个句子超过最大长度，按逗号拆分
            if (utf8Length(sentence) > static_cast<size_t>(max_length)) {
                std::string sub_remaining = sentence;
                while (!sub_remaining.empty()) {
                    std::string sub_sentence;
                    size_t comma_pos = sub_remaining.find(comma);
                    
                    if (comma_pos == std::string::npos) {
                        sub_sentence = sub_remaining;
                        sub_remaining.clear();
                    } else {
                        sub_sentence = sub_remaining.substr(0, comma_pos + 3);
                        sub_remaining = sub_remaining.substr(comma_pos + 3);
                    }
                    
                    if (utf8Length(current_message) + utf8Length(sub_sentence) > static_cast<size_t>(max_length)) {
                        if (!current_message.empty()) {
                            messages.push_back(current_message);
                            current_message.clear();
                        }
                    }
                    current_message += sub_sentence;
                }
            } else {
                current_message = sentence;
            }
        } else {
            current_message += sentence;
        }
    }
    
    if (!current_message.empty()) {
        messages.push_back(current_message);
    }
    
    // 兜底：按UTF-8字符边界强制拆分（不会切断多字节字符）
    if (messages.empty()) {
        std::string current;
        size_t char_count = 0;
        for (size_t i = 0; i < response.size();) {
            unsigned char c = response[i];
            size_t char_len = 1;
            if (c < 0x80) char_len = 1;
            else if ((c & 0xE0) == 0xC0) char_len = 2;
            else if ((c & 0xF0) == 0xE0) char_len = 3;
            else if ((c & 0xF8) == 0xF0) char_len = 4;
            
            if (char_count >= static_cast<size_t>(max_length)) {
                messages.push_back(current);
                current.clear();
                char_count = 0;
            }
            current += response.substr(i, char_len);
            i += char_len;
            char_count++;
        }
        if (!current.empty()) {
            messages.push_back(current);
        }
    }
    
    return messages;
}

/**
 * @brief 设置AI配置
 */
void AiService::setConfig(const AIServiceConfig& config) {
    config_ = config;
}

/**
 * @brief 获取AI配置
 */
const AIServiceConfig& AiService::getConfig() const {
    return config_;
}

/**
 * @brief 更新聊天AI设置
 */
bool AiService::updateChatAISettings(const std::string& chat_key, const std::string& nickname, 
                                      int tone, int priority, const std::string& updated_by) {
    ChatAISettingDAO dao(db_);
    ChatAISetting setting;
    setting.chat_key = chat_key;
    setting.nickname = nickname;
    setting.tone = tone;
    setting.priority = priority;
    setting.updated_by = updated_by;
    
    return dao.insertOrUpdate(setting);
}

/**
 * @brief 获取聊天AI设置
 */
bool AiService::getChatAISettings(const std::string& chat_key, std::string& nickname, 
                                   int& tone, int& priority) {
    ChatAISettingDAO dao(db_);
    ChatAISetting setting;
    
    if (dao.findByChatKey(chat_key, setting)) {
        nickname = setting.nickname;
        tone = setting.tone;
        priority = setting.priority;
        return true;
    }
    
    // 返回默认值
    nickname = "AI助手";
    tone = 0;
    priority = 1;
    return false;
}

/**
 * @brief 清理过期缓存
 */
int AiService::cleanExpiredCache() {
    int count = 0;
    std::time_t now = std::time(nullptr);
    std::time_t expire_time = now - (CACHE_EXPIRE_HOURS * 3600);
    
    // 清理内存缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        auto it = response_cache_.begin();
        while (it != response_cache_.end()) {
            if (it->second.second < expire_time) {
                it = response_cache_.erase(it);
                count++;
            } else {
                ++it;
            }
        }
    }
    
    // 清理数据库缓存
    char sql[256];
    snprintf(sql, sizeof(sql),
             "DELETE FROM ai_cache WHERE create_time < DATE_SUB(NOW(), INTERVAL %d HOUR)",
             CACHE_EXPIRE_HOURS);
    
    if (db_.execute(sql)) {
        std::cout << "Cleaned " << count << " expired AI cache entries" << std::endl;
    }
    
    return count;
}

/**
 * @brief 获取缓存的AI回复
 */
bool AiService::getCachedResponse(const std::string& question, std::string& response) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    // 1) 本地内存缓存
    auto it = response_cache_.find(question);
    if (it != response_cache_.end()) {
        std::time_t now = std::time(nullptr);
        if (now - it->second.second < CACHE_EXPIRE_HOURS * 3600) {
            response = it->second.first;
            return true;
        }
    }

    // 2) Redis 缓存（更快）
    try {
        std::string key = "ai:hot_question:" + util::md5(question);
        if (RedisClient::getInstance().isConnected()) {
            std::string r = RedisClient::getInstance().get(key);
            if (!r.empty()) {
                response = r;
                // 更新内存缓存
                response_cache_[question] = std::make_pair(response, std::time(nullptr));
                return true;
            }
        }
    } catch (...) {}
    
    // 从数据库查找
    char sql[4096];
    std::string safe_question = db_.escapeString(question);
    snprintf(sql, sizeof(sql),
             "SELECT response FROM ai_cache WHERE question='%s' "
             "AND create_time > DATE_SUB(NOW(), INTERVAL %d HOUR) "
             "ORDER BY create_time DESC LIMIT 1",
             safe_question.c_str(), CACHE_EXPIRE_HOURS);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row && row[0]) {
        response = row[0];
        
        // 更新内存缓存
        response_cache_[question] = std::make_pair(response, std::time(nullptr));
        
        db_.freeResult(res);
        // 同时将结果写入 Redis（加速后续查询）
        try {
            std::string key = "ai:hot_question:" + util::md5(question);
            RedisClient::getInstance().setex(key, CACHE_EXPIRE_HOURS * 3600, response);
        } catch (...) {
            // 忽略 Redis 失败
        }
        return true;
    }
    
    db_.freeResult(res);
    return false;
}

/**
 * @brief 缓存AI回复
 */
bool AiService::cacheResponse(const std::string& question, const std::string& response) {
    // 更新内存缓存
    {
        std::lock_guard<std::mutex> lock(cache_mutex_);
        response_cache_[question] = std::make_pair(response, std::time(nullptr));
    }
    // 写入 Redis（避开数据库写入以减少延迟）
    try {
        std::string key = "ai:hot_question:" + util::md5(question);
        if (RedisClient::getInstance().isConnected()) {
            RedisClient::getInstance().setex(key, CACHE_EXPIRE_HOURS * 3600, response);
        }
    } catch (...) {
        // 忽略 Redis 写入错误
    }
    return true;
}

/**
 * @brief 创建AI缓存表
 */
bool AiService::createAICacheTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS ai_cache ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "question TEXT NOT NULL,"
        "response TEXT NOT NULL,"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "INDEX idx_create_time (create_time)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 创建AI日志表
 */
bool AiService::createAILogTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS ai_log ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "request_id VARCHAR(64) NOT NULL,"
        "user_id VARCHAR(32),"
        "question TEXT,"
        "response TEXT,"
        "success TINYINT DEFAULT 0,"
        "error_msg VARCHAR(256),"
        "response_time INT DEFAULT 0,"
        "create_time DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "INDEX idx_request_id (request_id),"
        "INDEX idx_user_id (user_id),"
        "INDEX idx_create_time (create_time)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 创建聊天AI设置表
 */
bool AiService::createChatAISettingsTable() {
    std::string sql = 
        "CREATE TABLE IF NOT EXISTS chat_ai_settings ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "chat_key VARCHAR(64) NOT NULL UNIQUE,"
        "nickname VARCHAR(32) DEFAULT 'AI助手',"
        "tone TINYINT DEFAULT 0,"
        "priority TINYINT DEFAULT 1,"
        "updated_by VARCHAR(32) DEFAULT '',"
        "update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "INDEX idx_chat_key (chat_key)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    
    return db_.execute(sql);
}

/**
 * @brief 生成请求ID
 */
std::string AiService::generateRequestId() {
    return "AI_" + util::generateUUID().substr(0, 12);
}

/**
 * @brief 记录AI调用日志
 */
void AiService::logAICall(const std::string& request_id, const std::string& user_id, 
                           const std::string& question, const std::string& response, 
                           bool success, const std::string& error_msg, int response_time) {
    // 使用 Database::escapeString 以避免 SQL 注入并确保持有 DB 互斥锁
    std::string esc_request_id = db_.escapeString(request_id);
    std::string esc_user_id = db_.escapeString(user_id);
    std::string esc_question = db_.escapeString(question);
    std::string esc_response = db_.escapeString(response);
    std::string esc_error = db_.escapeString(error_msg);

    char sql[16384];
    snprintf(sql, sizeof(sql),
             "INSERT INTO ai_log (request_id, user_id, question, response, success, error_msg, response_time, create_time) "
             "VALUES ('%s', '%s', '%s', '%s', %d, '%s', %d, NOW())",
             esc_request_id.c_str(), esc_user_id.c_str(),
             esc_question.c_str(), esc_response.c_str(),
             success ? 1 : 0, esc_error.c_str(), response_time);

    if (!db_.execute(sql)) {
        std::cerr << "[AI] Failed to insert ai_log: " << mysql_error(db_.getConnection()) << std::endl;
    }
}

/**
 * @brief 发送AI回复给用户
 */
void AiService::sendAIResponse(const std::string& ai_nickname, const std::string& target_id, 
                                const std::vector<std::string>& messages, bool is_group,
                                const std::string& origin_user_id, const std::string& chat_key) {
    std::cout << "[AI] Sending AI response to " << target_id << ", messages=" << messages.size() << std::endl;
    ChatService& chat_service = ChatService::getInstance();

    // 获取目标用户的数字ID（用于保存聊天记录）
    uint64_t origin_user_id_num = getUserIdNum(db_, origin_user_id);

    for (const auto& message : messages) {
        if (is_group) {
            // 群聊：to_user_id 为群ID，extra 包含 group_id 和 chat_key
            std::ostringstream oss;
            oss << "{\"type\":34"
                << ",\"from_user_id\":\"" << escapeJson(ai_nickname) << "\""
                << ",\"to_user_id\":\"" << target_id << "\""
                << ",\"content\":\"" << escapeJson(message) << "\""
                << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"group_id\\\":\\\"" << target_id 
                << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(chat_key) << "\\\"}\""
                << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";

            std::string msg_str = oss.str();
            std::ostringstream wrapper;
            wrapper << "{\"type\":34,\"code\":0,\"msg\":\"\",\"data\":" << msg_str 
                    << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";

            chat_service.broadcastToGroup(target_id, wrapper.str());
        } else {
            // 私聊：需要为双方生成不同的 chat_key
            // 对于发起者 (origin_user_id)，chat_key 是 "single:target_id"
            // 对于接收者 (target_id)，chat_key 是 "single:origin_user_id"
            std::string origin_chat_key = chat_key;  // 发起者的 chat_key
            std::string target_chat_key = "single:" + origin_user_id;  // 接收者的 chat_key

            // 构建给发起者的消息
            std::ostringstream oss_origin;
            oss_origin << "{\"type\":40"
                << ",\"from_user_id\":\"" << escapeJson(ai_nickname) << "\""
                << ",\"to_user_id\":\"" << origin_user_id << "\""
                << ",\"content\":\"" << escapeJson(message) << "\""
                << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"chat_key\\\":\\\"" << escapeJson(origin_chat_key) << "\\\"}\""
                << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";

            // 构建给接收者的消息
            std::ostringstream oss_target;
            oss_target << "{\"type\":40"
                << ",\"from_user_id\":\"" << escapeJson(ai_nickname) << "\""
                << ",\"to_user_id\":\"" << target_id << "\""
                << ",\"content\":\"" << escapeJson(message) << "\""
                << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"chat_key\\\":\\\"" << escapeJson(target_chat_key) << "\\\"}\""
                << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";

            // 包装并发送给发起者
            std::ostringstream wrapper_origin;
            wrapper_origin << "{\"type\":40,\"code\":0,\"msg\":\"\",\"data\":" << oss_origin.str() 
                          << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";
            chat_service.broadcastToUser(origin_user_id, wrapper_origin.str());

            // 包装并发送给接收者
            std::ostringstream wrapper_target;
            wrapper_target << "{\"type\":40,\"code\":0,\"msg\":\"\",\"data\":" << oss_target.str() 
                          << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";
            chat_service.broadcastToUser(target_id, wrapper_target.str());
        }

        // 保存到聊天记录
        ChatRecord record;
        // 获取AI用户的数字ID（从user表中查询user_id='ai'的记录）
        static uint64_t ai_user_id_num = 0;
        static bool ai_user_id_initialized = false;
        if (!ai_user_id_initialized) {
            ai_user_id_num = getUserIdNum(db_, "ai");
            ai_user_id_initialized = true;
        }
        record.sender_id = ai_user_id_num;  // 使用AI用户的实际ID
        record.content = message;
        record.is_ai = 1;

        if (is_group) {
            uint64_t group_id_num = getGroupNumId(db_, target_id);
            record.group_id = group_id_num;
            record.receiver_id = 0;
        } else {
            record.group_id = 0;
            record.receiver_id = origin_user_id_num;
        }

        ChatRecordDAO record_dao(db_);
        record_dao.insert(record);

        // 添加延迟，模拟逐条发送
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

/**
 * @brief 估算文本的Token数（近似值）
 * 中文约1.5 tokens/字，英文约1.3 tokens/词，数字符号约0.5 tokens/字符
 */
int AiService::estimateTokens(const std::string& text) {
    if (text.empty()) return 0;
    
    int chinese_chars = 0;
    int ascii_chars = 0;
    int digit_count = 0;
    
    for (size_t i = 0; i < text.size();) {
        unsigned char c = text[i];
        if (c < 0x80) {
            if (c >= '0' && c <= '9') digit_count++;
            else ascii_chars++;
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            chinese_chars++;
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            i += 4;
        } else {
            i += 1;
        }
    }
    
    int english_tokens = static_cast<int>((ascii_chars / 5) * 1.3 + digit_count * 0.5);
    int chinese_tokens = static_cast<int>(chinese_chars * 1.5);
    
    return english_tokens + chinese_tokens + 1;
}

/**
 * @brief 按Token预算裁剪上下文（滑动窗口，优先保留最近消息）
 */
void AiService::trimContextByTokenBudget(std::vector<ContextMessage>& context, int max_tokens) {
    if (context.empty() || max_tokens <= 0) return;
    
    int total_tokens = 0;
    for (const auto& msg : context) {
        total_tokens += estimateTokens(msg.content);
    }
    
    if (total_tokens <= max_tokens) return;
    
    // 至少保留最近2轮对话（4条消息）
    size_t min_keep = std::min(static_cast<size_t>(4), context.size());
    
    while (!context.empty() && context.size() > min_keep) {
        int oldest_tokens = estimateTokens(context.front().content);
        total_tokens -= oldest_tokens;
        context.erase(context.begin());
        
        if (total_tokens <= max_tokens) break;
    }
    
    std::cout << "[AI] Context trimmed to " << context.size() 
              << " messages, ~" << total_tokens << " tokens" << std::endl;
}

/**
 * @brief 获取多轮对话上下文
 */
std::vector<ContextMessage> AiService::getContextMessages(const std::string& user_id, 
                                                            const std::string& target_id, 
                                                            bool is_group) {
    std::vector<ContextMessage> context;
    
    static uint64_t ai_user_id_num = 0;
    static bool ai_user_id_initialized = false;
    if (!ai_user_id_initialized) {
        ai_user_id_num = getUserIdNum(db_, "ai");
        ai_user_id_initialized = true;
    }
    
    std::vector<ChatRecord> records;
    
    if (is_group) {
        uint64_t group_id_num = getGroupNumId(db_, target_id);
        if (group_id_num == 0) return context;
        ChatRecordDAO dao(db_);
        records = dao.findByGroup(group_id_num, config_.context_message_count * 2);
    } else {
        uint64_t user_id_num = getUserIdNum(db_, user_id);
        if (user_id_num == 0 || ai_user_id_num == 0) return context;
        ChatRecordDAO dao(db_);
        records = dao.findByUserPair(user_id_num, ai_user_id_num, config_.context_message_count * 2);
    }
    
    std::reverse(records.begin(), records.end());
    
    for (const auto& record : records) {
        if (record.is_recalled) continue;
        ContextMessage msg;
        msg.role = record.is_ai ? "assistant" : "user";
        msg.content = record.content;
        context.push_back(msg);
    }
    
    trimContextByTokenBudget(context, config_.max_context_tokens);
    
    std::cout << "[AI] Context built: " << context.size() << " messages" << std::endl;
    return context;
}

/**
 * @brief 发送流式开始消息
 */
void AiService::sendAIStreamStart(const std::string& stream_id, const std::string& ai_nickname,
                                   const std::string& target_id, bool is_group,
                                   const std::string& origin_user_id, const std::string& chat_key) {
    ChatService& chat_service = ChatService::getInstance();
    std::string ts = util::getCurrentTime();
    
    if (is_group) {
        std::ostringstream oss;
        oss << "{\"type\":53,\"code\":0,\"msg\":\"\",\"data\":{"
            << "\"type\":53,\"from_user_id\":\"" << escapeJson(ai_nickname) << "\""
            << ",\"to_user_id\":\"" << target_id << "\",\"content\":\"\""
            << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"stream_id\\\":\\\"" << escapeJson(stream_id) 
            << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(chat_key) 
            << "\\\",\\\"ai_nickname\\\":\\\"" << escapeJson(ai_nickname) << "\\\"}\""
            << ",\"timestamp\":\"" << ts << "\"},\"timestamp\":\"" << ts << "\"}";
        chat_service.broadcastToGroup(target_id, oss.str());
    } else {
        std::string origin_chat_key = chat_key;
        std::string target_chat_key = "single:" + origin_user_id;
        
        auto buildMsg = [&](const std::string& to_uid, const std::string& ck) -> std::string {
            std::ostringstream oss;
            oss << "{\"type\":53,\"code\":0,\"msg\":\"\",\"data\":{"
                << "\"type\":53,\"from_user_id\":\"" << escapeJson(ai_nickname) << "\""
                << ",\"to_user_id\":\"" << to_uid << "\",\"content\":\"\""
                << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"stream_id\\\":\\\"" << escapeJson(stream_id) 
                << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(ck) << "\\\"}\""
                << ",\"timestamp\":\"" << ts << "\"},\"timestamp\":\"" << ts << "\"}";
            return oss.str();
        };
        chat_service.broadcastToUser(origin_user_id, buildMsg(origin_user_id, origin_chat_key));
        if (target_id != "ai" && target_id != origin_user_id) {
            chat_service.broadcastToUser(target_id, buildMsg(target_id, target_chat_key));
        }
    }
}

/**
 * @brief 发送流式增量消息
 */
void AiService::sendAIStreamChunk(const std::string& stream_id, const std::string& delta,
                                  const std::string& target_id, bool is_group,
                                  const std::string& origin_user_id, const std::string& chat_key) {
    ChatService& chat_service = ChatService::getInstance();
    std::string ts = util::getCurrentTime();
    
    if (is_group) {
        std::ostringstream oss;
        oss << "{\"type\":54,\"code\":0,\"msg\":\"\",\"data\":{"
            << "\"type\":54,\"from_user_id\":\"AI\",\"to_user_id\":\"" << target_id << "\""
            << ",\"content\":\"" << escapeJson(delta) << "\""
            << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"stream_id\\\":\\\"" << escapeJson(stream_id) 
            << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(chat_key) << "\\\"}\""
            << ",\"timestamp\":\"" << ts << "\"},\"timestamp\":\"" << ts << "\"}";
        chat_service.broadcastToGroup(target_id, oss.str());
    } else {
        std::string origin_chat_key = chat_key;
        std::string target_chat_key = "single:" + origin_user_id;
        
        auto buildMsg = [&](const std::string& to_uid, const std::string& ck) -> std::string {
            std::ostringstream oss;
            oss << "{\"type\":54,\"code\":0,\"msg\":\"\",\"data\":{"
                << "\"type\":54,\"from_user_id\":\"AI\",\"to_user_id\":\"" << to_uid << "\""
                << ",\"content\":\"" << escapeJson(delta) << "\""
                << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"stream_id\\\":\\\"" << escapeJson(stream_id) 
                << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(ck) << "\\\"}\""
                << ",\"timestamp\":\"" << ts << "\"},\"timestamp\":\"" << ts << "\"}";
            return oss.str();
        };
        chat_service.broadcastToUser(origin_user_id, buildMsg(origin_user_id, origin_chat_key));
        if (target_id != "ai" && target_id != origin_user_id) {
            chat_service.broadcastToUser(target_id, buildMsg(target_id, target_chat_key));
        }
    }
}

/**
 * @brief 发送流式结束消息
 */
void AiService::sendAIStreamEnd(const std::string& stream_id, const std::string& target_id,
                                bool is_group, const std::string& origin_user_id,
                                const std::string& chat_key, bool success) {
    ChatService& chat_service = ChatService::getInstance();
    std::string ts = util::getCurrentTime();
    std::string success_str = success ? "true" : "false";
    
    if (is_group) {
        std::ostringstream oss;
        oss << "{\"type\":55,\"code\":0,\"msg\":\"\",\"data\":{"
            << "\"type\":55,\"from_user_id\":\"AI\",\"to_user_id\":\"" << target_id << "\",\"content\":\"\""
            << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"stream_id\\\":\\\"" << escapeJson(stream_id) 
            << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(chat_key) 
            << "\\\",\\\"success\\\":" << success_str << "}\""
            << ",\"timestamp\":\"" << ts << "\"},\"timestamp\":\"" << ts << "\"}";
        chat_service.broadcastToGroup(target_id, oss.str());
    } else {
        std::string origin_chat_key = chat_key;
        std::string target_chat_key = "single:" + origin_user_id;
        
        auto buildMsg = [&](const std::string& to_uid, const std::string& ck) -> std::string {
            std::ostringstream oss;
            oss << "{\"type\":55,\"code\":0,\"msg\":\"\",\"data\":{"
                << "\"type\":55,\"from_user_id\":\"AI\",\"to_user_id\":\"" << to_uid << "\",\"content\":\"\""
                << ",\"extra\":\"{\\\"is_ai\\\":true,\\\"stream_id\\\":\\\"" << escapeJson(stream_id) 
                << "\\\",\\\"chat_key\\\":\\\"" << escapeJson(ck) 
                << "\\\",\\\"success\\\":" << success_str << "}\""
                << ",\"timestamp\":\"" << ts << "\"},\"timestamp\":\"" << ts << "\"}";
            return oss.str();
        };
        chat_service.broadcastToUser(origin_user_id, buildMsg(origin_user_id, origin_chat_key));
        if (target_id != "ai" && target_id != origin_user_id) {
            chat_service.broadcastToUser(target_id, buildMsg(target_id, target_chat_key));
        }
    }
}

/**
 * @brief 异步处理AI请求
 */
void AiService::asyncProcessRequest(const AIRequest& request) {
    std::cout << "[AI] Processing request: " << request.request_id << std::endl;
    auto start_time = std::chrono::steady_clock::now();
    
    // 1. 获取聊天AI设置
    std::string chat_key = getJsonValue(request.extra, "chatKey");
    if (chat_key.empty()) {
        if (request.is_group) {
            chat_key = "group:" + request.target_id;
        } else {
            chat_key = "single:" + request.target_id;
        }
    }
    
    std::string ai_nickname;
    int tone = 0;
    int priority = 0;
    if (!getChatAISettings(chat_key, ai_nickname, tone, priority)) {
        ai_nickname = "AI助手";
    }
    
    // 2. 获取多轮对话上下文（含Token预算裁剪）
    std::vector<ContextMessage> context = getContextMessages(request.user_id, request.target_id, request.is_group);
    
    bool success = false;
    std::string full_response;
    
    if (config_.enable_stream) {
        // === 流式路径（带分段） ===
        std::cout << "[AI] Starting streaming response with segmentation..." << std::endl;
        
        // 分段状态
        const int MIN_SEGMENT_CHARS = 30;   // 最小段长（UTF-8字符数）
        const int MAX_SEGMENT_CHARS = 100;  // 最大段长
        const int FLUSH_THRESHOLD = 3;      // 缓冲刷新阈值（字符数）
        
        int segment_index = 0;
        std::string current_seg_id = request.request_id + "_seg0";
        std::string seg_buffer;              // 当前段缓冲区
        int seg_char_count = 0;              // 当前段已发送字符数
        
        // UTF-8字符计数辅助
        auto utf8Len = [](const std::string& s) -> int {
            int n = 0;
            for (size_t i = 0; i < s.size();) {
                unsigned char c = s[i];
                if (c < 0x80) i += 1;
                else if ((c & 0xE0) == 0xC0) i += 2;
                else if ((c & 0xF0) == 0xE0) i += 3;
                else if ((c & 0xF8) == 0xF0) i += 4;
                else i += 1;
                n++;
            }
            return n;
        };
        
        // 在缓冲区中查找分段位置，返回分段点（该位置之前的文本归当前段）
        // 返回 npos 表示不需要分段
        auto findSplitPoint = [&](const std::string& buf, int sent_chars) -> size_t {
            int total_chars = sent_chars + utf8Len(buf);
            
            // 优先级1: 段落断行 \n\n
            size_t pos = buf.find("\n\n");
            if (pos != std::string::npos) {
                return pos + 2;
            }
            
            // 优先级2: 编号列表项 \n\d+[.、]
            {
                size_t p = buf.find("\n");
                while (p != std::string::npos && p + 2 < buf.size()) {
                    char next = buf[p + 1];
                    if ((next >= '0' && next <= '9') && 
                        (buf[p + 2] == '.' || buf[p + 2] == '、')) {
                        if (sent_chars > 0 || p > 0) {
                            return p + 1;  // 换行符归前一段
                        }
                    }
                    p = buf.find("\n", p + 1);
                }
            }
            
            // 优先级3: 句末标点 。！？ (当前段≥最小段长时)
            if (total_chars >= MIN_SEGMENT_CHARS) {
                const char* markers[] = {"。", "！", "？", "!", "?"};
                int marker_lens[] = {3, 3, 3, 1, 1};
                size_t best = std::string::npos;
                int best_len = 0;
                for (int i = 0; i < 5; i++) {
                    size_t p = buf.rfind(markers[i]);
                    if (p != std::string::npos && (best == std::string::npos || p > best)) {
                        best = p;
                        best_len = marker_lens[i];
                    }
                }
                if (best != std::string::npos) {
                    return best + best_len;
                }
                
                // 优先级4: 换行 \n
                size_t nl = buf.rfind('\n');
                if (nl != std::string::npos) {
                    return nl + 1;
                }
            }
            
            // 优先级5: 逗号 ，, (当前段≥50字时)
            if (total_chars >= 50) {
                size_t p1 = buf.rfind("，");
                size_t p2 = buf.rfind(",");
                size_t best = std::string::npos;
                int best_len = 0;
                if (p1 != std::string::npos && (best == std::string::npos || p1 > best)) {
                    best = p1; best_len = 3;
                }
                if (p2 != std::string::npos && (best == std::string::npos || p2 > best)) {
                    best = p2; best_len = 1;
                }
                if (best != std::string::npos) {
                    return best + best_len;
                }
            }
            
            // 优先级6: 强制截断 (当前段≥最大段长时)
            if (total_chars >= MAX_SEGMENT_CHARS) {
                // 在最大段长位置按UTF-8字符边界截断
                int char_pos = 0;
                size_t byte_pos = 0;
                while (byte_pos < buf.size() && char_pos < MAX_SEGMENT_CHARS - sent_chars) {
                    unsigned char c = buf[byte_pos];
                    if (c < 0x80) byte_pos += 1;
                    else if ((c & 0xE0) == 0xC0) byte_pos += 2;
                    else if ((c & 0xF0) == 0xE0) byte_pos += 3;
                    else if ((c & 0xF8) == 0xF0) byte_pos += 4;
                    else byte_pos += 1;
                    char_pos++;
                }
                return byte_pos;
            }
            
            return std::string::npos;
        };
        
        // 发送初始段START
        sendAIStreamStart(current_seg_id, ai_nickname, request.target_id, 
                          request.is_group, request.user_id, chat_key);
        
        bool chunk_sent = false;
        
        bool stream_ok = callAIAPIStream(request.question, context,
                                          static_cast<AITone>(tone), 
                                          static_cast<AIPriority>(priority),
                                          [&](const std::string& delta, bool is_done) {
                                              if (is_done) {
                                                  // 流结束：flush剩余缓冲区
                                                  if (!seg_buffer.empty()) {
                                                      sendAIStreamChunk(current_seg_id, seg_buffer, 
                                                                        request.target_id, request.is_group, 
                                                                        request.user_id, chat_key);
                                                      seg_buffer.clear();
                                                  }
                                                  return;
                                              }
                                              
                                              if (delta.empty()) return;
                                              
                                              full_response += delta;
                                              seg_buffer += delta;
                                              chunk_sent = true;
                                              
                                              // 检测分段
                                              size_t split_pos = findSplitPoint(seg_buffer, seg_char_count);
                                              
                                              if (split_pos != std::string::npos && split_pos < seg_buffer.size()) {
                                                  // 执行分段：split_pos之前的文本发给当前段
                                                  std::string to_current = seg_buffer.substr(0, split_pos);
                                                  std::string to_next = seg_buffer.substr(split_pos);
                                                  
                                                  if (!to_current.empty()) {
                                                      sendAIStreamChunk(current_seg_id, to_current, 
                                                                        request.target_id, request.is_group, 
                                                                        request.user_id, chat_key);
                                                  }
                                                  
                                                  // 结束当前段
                                                  sendAIStreamEnd(current_seg_id, request.target_id, 
                                                                  request.is_group, request.user_id, 
                                                                  chat_key, true);
                                                  
                                                  // 开始新段
                                                  segment_index++;
                                                  current_seg_id = request.request_id + "_seg" + std::to_string(segment_index);
                                                  sendAIStreamStart(current_seg_id, ai_nickname, request.target_id, 
                                                                    request.is_group, request.user_id, chat_key);
                                                  seg_char_count = 0;
                                                  
                                                  // 剩余文本作为新段的缓冲
                                                  seg_buffer = to_next;
                                                  seg_char_count += utf8Len(to_next);
                                                  
                                                  // 立即flush新段的缓冲（如果有内容）
                                                  if (utf8Len(seg_buffer) >= FLUSH_THRESHOLD) {
                                                      sendAIStreamChunk(current_seg_id, seg_buffer, 
                                                                        request.target_id, request.is_group, 
                                                                        request.user_id, chat_key);
                                                      seg_char_count += utf8Len(seg_buffer);
                                                      seg_buffer.clear();
                                                  }
                                              } else if (utf8Len(seg_buffer) >= FLUSH_THRESHOLD) {
                                                  // 无分段点，达到刷新阈值时flush
                                                  sendAIStreamChunk(current_seg_id, seg_buffer, 
                                                                    request.target_id, request.is_group, 
                                                                    request.user_id, chat_key);
                                                  seg_char_count += utf8Len(seg_buffer);
                                                  seg_buffer.clear();
                                              }
                                          });
        
        if (stream_ok) {
            // 流式成功：结束最后一段
            success = true;
            sendAIStreamEnd(current_seg_id, request.target_id, request.is_group, 
                            request.user_id, chat_key, true);
            std::cout << "[AI] Stream completed with " << (segment_index + 1) 
                      << " segments, response length=" << full_response.length() << std::endl;
        } else if (!chunk_sent) {
            // 连接失败，未发出任何chunk → 回退非流式
            std::cerr << "[AI] Stream failed (no chunks), falling back to non-stream..." << std::endl;
            sendAIStreamEnd(current_seg_id, request.target_id, request.is_group, 
                            request.user_id, chat_key, false);
            
            std::string response;
            if (callAIAPI(request.question, static_cast<AITone>(tone), 
                         static_cast<AIPriority>(priority), response, context)) {
                success = true;
                full_response = response;
                std::vector<std::string> messages = splitResponse(response, config_.max_message_length);
                sendAIResponse(ai_nickname, request.target_id, messages, request.is_group, 
                              request.user_id, chat_key);
            } else {
                full_response = response;
                std::vector<std::string> error_messages = {response};
                sendAIResponse(ai_nickname, request.target_id, error_messages, request.is_group, 
                              request.user_id, chat_key);
            }
        } else {
            // 中途断开，已有部分chunk
            std::cerr << "[AI] Stream interrupted, partial response saved" << std::endl;
            sendAIStreamEnd(current_seg_id, request.target_id, request.is_group, 
                            request.user_id, chat_key, false);
            success = !full_response.empty();
        }
    } else {
        // === 非流式路径 ===
        std::cout << "[AI] Calling API (non-stream)..." << std::endl;
        std::string response;
        success = callAIAPI(request.question, static_cast<AITone>(tone), 
                           static_cast<AIPriority>(priority), response, context);
        full_response = response;
        
        if (success) {
            std::vector<std::string> messages = splitResponse(response, config_.max_message_length);
            sendAIResponse(ai_nickname, request.target_id, messages, request.is_group, 
                          request.user_id, chat_key);
        } else {
            std::vector<std::string> error_messages = {response};
            sendAIResponse(ai_nickname, request.target_id, error_messages, request.is_group, 
                          request.user_id, chat_key);
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    int response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // 5. 保存完整回复为单条ChatRecord（流式模式）
    if (success && !full_response.empty() && config_.enable_stream) {
        static uint64_t ai_user_id_num = 0;
        static bool ai_user_id_initialized = false;
        if (!ai_user_id_initialized) {
            ai_user_id_num = getUserIdNum(db_, "ai");
            ai_user_id_initialized = true;
        }
        
        uint64_t origin_user_id_num = getUserIdNum(db_, request.user_id);
        
        ChatRecord record;
        record.sender_id = ai_user_id_num;
        record.content = full_response;
        record.is_ai = 1;
        
        if (request.is_group) {
            uint64_t group_id_num = getGroupNumId(db_, request.target_id);
            record.group_id = group_id_num;
            record.receiver_id = 0;
        } else {
            record.group_id = 0;
            record.receiver_id = origin_user_id_num;
        }
        
        ChatRecordDAO record_dao(db_);
        record_dao.insert(record);
    }
    
    // 6. 记录日志
    logAICall(request.request_id, request.user_id, request.question, full_response, 
              success, success ? "" : full_response, response_time);
    
    // 7. 从待处理队列移除
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        pending_requests_.erase(request.request_id);
    }
    
    std::cout << "[AI] Request processed: " << request.request_id 
              << " in " << response_time << "ms, success=" << success << std::endl;
}

/**
 * @brief 调用HTTP请求
 */
bool AiService::httpPost(const std::string& url, const std::map<std::string, std::string>& headers, 
                          const std::string& body, std::string& response, int timeout) {
#ifdef USE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config_.connect_timeout);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    
    // 设置请求头
    struct curl_slist* chunk = nullptr;
    for (const auto& header : headers) {
        std::string header_str = header.first + ": " + header.second;
        chunk = curl_slist_append(chunk, header_str.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    
    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);

    std::cout << "[AI] curl result: res=" << res << ", http_code=" << http_code
              << ", response_len=" << response.size() << std::endl;

    // 要求 libcurl 执行成功、HTTP 200 且响应体非空才视为成功
    if (res == CURLE_OK && http_code >= 200 && http_code < 300 && !response.empty()) {
        return true;
    }

    // 记录失败原因到 stderr
    if (res != CURLE_OK) {
        std::cerr << "[AI] curl perform error: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cerr << "[AI] HTTP non-2xx or empty response: code=" << http_code << ", len=" << response.size() << std::endl;
    }

    return false;
#else
    // 简化实现：使用系统命令调用curl
    // 使用 mkstemp 创建唯一临时文件以避免并发冲突
    char body_template[] = "/tmp/ai_body_XXXXXX";
    int body_fd = mkstemp(body_template);
    if (body_fd == -1) {
        std::cerr << "[AI] Failed to create body temp file" << std::endl;
        return false;
    }

    // 将 body 写入临时文件
    ssize_t written = write(body_fd, body.c_str(), static_cast<size_t>(body.size()));
    (void)written;
    close(body_fd);

    char resp_template[] = "/tmp/ai_response_XXXXXX";
    int resp_fd = mkstemp(resp_template);
    if (resp_fd == -1) {
        std::cerr << "[AI] Failed to create response temp file" << std::endl;
        // 清理 body 文件
        ::unlink(body_template);
        return false;
    }
    close(resp_fd);

    std::ostringstream cmd;
    cmd << "curl -sS -X POST '" << url << "'"
        << " -H 'Content-Type: application/json'";

    for (const auto& header : headers) {
        if (header.first != "Content-Type") {
            cmd << " -H '" << header.first << ": " << header.second << "'";
        }
    }

    cmd << " -d @" << body_template
        << " -o " << resp_template
        << " --connect-timeout " << timeout
        << " --max-time " << (timeout + 5);

    std::cout << "[AI] Executing curl command..." << std::endl;
    std::cout << "[AI] Command: " << cmd.str().substr(0, 300) << "..." << std::endl;
    int ret = system(cmd.str().c_str());
    std::cout << "[AI] curl returned: " << ret << std::endl;

    // 读取并清理临时文件
    std::ifstream file(resp_template);
    if (ret == 0 && file.is_open()) {
        std::ostringstream oss;
        oss << file.rdbuf();
        response = oss.str();
        file.close();
        std::cout << "[AI] Response length: " << response.length() << std::endl;
    } else {
        std::cerr << "[AI] curl command failed or response file not readable: code=" << ret << std::endl;
    }

    // 删除临时文件
    ::unlink(body_template);
    ::unlink(resp_template);

    return (ret == 0 && !response.empty());
#endif
}

/**
 * @brief 解析AI API响应
 */
bool AiService::parseAIResponse(const std::string& response, std::string& content) {
    try {
        std::cout << "[AI] Parsing response, length=" << response.length() << std::endl;
        
        // 解析OpenAI格式的响应
        // {"choices":[{"message":{"content":"..."}}]}
        
        std::string choices = getJsonValue(response, "choices");
        std::cout << "[AI] Choices: " << (choices.empty() ? "empty" : choices.substr(0, 100)) << std::endl;
        
        if (choices.empty()) {
            // 尝试解析其他格式
            content = getJsonValue(response, "content");
            if (content.empty()) {
                content = getJsonValue(response, "response");
                if (content.empty()) {
                    content = getJsonValue(response, "text");
                }
            }
            std::cout << "[AI] Direct content: " << (content.empty() ? "empty" : content.substr(0, 100)) << std::endl;
            return !content.empty();
        }
        
        // 解析choices数组
        if (choices[0] == '[') {
            std::cout << "[AI] Choices is array, finding message..." << std::endl;
            // 找到第一个message对象
            size_t msg_pos = choices.find("\"message\"");
            std::cout << "[AI] message pos: " << msg_pos << std::endl;
            if (msg_pos != std::string::npos) {
                std::string message = choices.substr(msg_pos);
                std::cout << "[AI] message string: " << message.substr(0, 100) << std::endl;
                content = getJsonValue(message, "content");
                std::cout << "[AI] Content from message: " << (content.empty() ? "empty" : content.substr(0, 100)) << std::endl;
                return !content.empty();
            }
        }
        
        std::cerr << "[AI] Failed to parse choices" << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[AI] Parse error: " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "[AI] Unknown parse error" << std::endl;
        return false;
    }
}

/**
 * @brief 静态JSON值获取（供C回调使用）
 */
std::string AiService::getJsonStatic(const std::string& json, const std::string& key) {
    return JsonUtil::getString(json, key);
}

/**
 * @brief 解析SSE数据行
 * @param line SSE数据行（如 "data: {...}" 或 "data: [DONE]"）
 * @param content 输出的内容（delta.content）
 * @return 0=普通内容, 1=流结束([DONE]), -1=无效行
 */
int AiService::parseSSEData(const std::string& line, std::string& content) {
    content.clear();
    
    // 查找 "data:" 前缀
    size_t data_pos = line.find("data:");
    if (data_pos == std::string::npos) {
        return -1;
    }
    
    std::string payload = line.substr(data_pos + 5);  // 跳过 "data:"
    // 去除首尾空白
    size_t start = payload.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return -1;
    }
    size_t end = payload.find_last_not_of(" \t\r\n");
    payload = payload.substr(start, end - start + 1);
    
    if (payload == "[DONE]") {
        return 1;
    }
    
    if (payload.empty() || payload[0] != '{') {
        return -1;
    }
    
    // 解析 OpenAI 流式格式 {"choices":[{"delta":{"content":"..."}}]}
    std::string choices = getJsonValue(payload, "choices");
    if (choices.empty() || choices[0] != '[') {
        return -1;
    }
    
    size_t delta_pos = choices.find("\"delta\"");
    if (delta_pos == std::string::npos) {
        return -1;
    }
    
    std::string delta_str = choices.substr(delta_pos);
    content = getJsonValue(delta_str, "content");
    // content 可能为空（如 role 首次出现的 delta 无 content）
    return 0;
}

#ifdef USE_CURL
/**
 * @brief CURL流式写回调函数
 * 累积数据到缓冲区，按行处理SSE
 */
struct StreamContext {
    std::string buffer;
    AIStreamCallback callback;
    bool stream_started;  // 是否已经发出过内容
    bool stream_done;     // 流是否已经正常结束
};

static size_t StreamWriteCallback(void* contents, size_t size, size_t nmemb, StreamContext* ctx) {
    size_t total = size * nmemb;
    ctx->buffer.append((char*)contents, total);
    
    // 按行处理（SSE 格式每行以 \n 结尾）
    size_t line_end;
    while ((line_end = ctx->buffer.find('\n')) != std::string::npos) {
        std::string line = ctx->buffer.substr(0, line_end);
        ctx->buffer.erase(0, line_end + 1);
        
        // 跳过空行（SSE 事件之间用空行分隔）
        if (line.empty() || line == "\r") {
            continue;
        }
        
        std::string content;
        int result = 0;
        // 注意：parseSSEData 是 AiService 的实例方法，这里通过回调闭包间接调用
        // 由于 C 回调无法直接访问实例，改为在此处内联解析
        {
            size_t data_pos = line.find("data:");
            if (data_pos != std::string::npos) {
                std::string payload = line.substr(data_pos + 5);
                size_t ps = payload.find_first_not_of(" \t\r\n");
                if (ps != std::string::npos) {
                    size_t pe = payload.find_last_not_of(" \t\r\n");
                    payload = payload.substr(ps, pe - ps + 1);
                } else {
                    payload.clear();
                }
                
                if (payload == "[DONE]") {
                    result = 1;
                } else if (!payload.empty() && payload[0] == '{') {
                    // 解析 OpenAI 流式格式
                    std::string choices = AiService::getJsonStatic(payload, "choices");
                    if (!choices.empty() && choices[0] == '[') {
                        size_t delta_pos = choices.find("\"delta\"");
                        if (delta_pos != std::string::npos) {
                            std::string delta_str = choices.substr(delta_pos);
                            content = AiService::getJsonStatic(delta_str, "content");
                        }
                    }
                    result = 0;
                }
            }
        }
        
        if (result == 1) {
            ctx->stream_done = true;
            if (ctx->stream_started) {
                ctx->callback("", true);  // 流结束，无内容
            }
        } else if (result == 0 && !content.empty()) {
            ctx->stream_started = true;
            ctx->callback(content, false);
        }
    }
    
    return total;
}
#endif

/**
 * @brief 流式HTTP POST请求
 * @return 调用成功返回true，失败返回false（连接失败）
 */
bool AiService::httpPostStream(const std::string& url, const std::map<std::string, std::string>& headers,
                                const std::string& body, const AIStreamCallback& callback, int timeout) {
#ifdef USE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) {
        return false;
    }
    
    StreamContext ctx;
    ctx.callback = callback;
    ctx.stream_started = false;
    ctx.stream_done = false;
    ctx.buffer.clear();
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, StreamWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, config_.connect_timeout);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    
    struct curl_slist* chunk = nullptr;
    for (const auto& header : headers) {
        std::string header_str = header.first + ": " + header.second;
        chunk = curl_slist_append(chunk, header_str.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    
    CURLcode res = curl_easy_perform(curl);
    
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);
    
    if (res == CURLE_OK && http_code >= 200 && http_code < 300) {
        // 如果流正常结束（[DONE]），ctx.stream_done 应已为 true
        // 如果流没有正常结束但也没发出任何内容，认为失败
        if (!ctx.stream_started && !ctx.stream_done) {
            std::cerr << "[AI] Stream returned no content" << std::endl;
            return false;
        }
        return true;
    }
    
    if (res != CURLE_OK) {
        std::cerr << "[AI] curl stream error: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cerr << "[AI] HTTP non-2xx in stream: code=" << http_code << std::endl;
    }
    return false;
#else
    std::cerr << "[AI] Streaming requires USE_CURL build" << std::endl;
    return false;
#endif
}
