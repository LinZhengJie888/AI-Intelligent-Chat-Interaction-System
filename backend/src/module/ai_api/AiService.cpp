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
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\'': result += "''"; break;  // SQL单引号转义
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c; break;
        }
    }
    return result;
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
 * @brief 获取JSON值辅助函数
 */
static std::string getJsonValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    pos++;
    
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.size()) return "";
    
    std::string value;
    if (json[pos] == '"') {
        // 字符串值 - 正确处理转义引号
        pos++;
        size_t end = pos;
        while (end < json.size()) {
            if (json[end] == '\\' && end + 1 < json.size()) {
                end += 2; // 跳过转义字符
                continue;
            }
            if (json[end] == '"') break;
            end++;
        }
        if (end >= json.size()) return "";
        value = json.substr(pos, end - pos);
        // 处理转义字符
        std::string unescaped;
        for (size_t i = 0; i < value.size(); i++) {
            if (value[i] == '\\' && i + 1 < value.size()) {
                switch (value[i + 1]) {
                    case '"': unescaped += '"'; i++; break;
                    case '\\': unescaped += '\\'; i++; break;
                    case 'n': unescaped += '\n'; i++; break;
                    case 'r': unescaped += '\r'; i++; break;
                    case 't': unescaped += '\t'; i++; break;
                    default: unescaped += value[i]; break;
                }
            } else {
                unescaped += value[i];
            }
        }
        value = unescaped;
    } else if (json[pos] == '{' || json[pos] == '[') {
        int bracket_count = 0;
        char target_bracket = (json[pos] == '{') ? '}' : ']';
        size_t start = pos;
        for (; pos < json.size(); pos++) {
            if (json[pos] == '\\') {
                pos++; // 跳过转义字符
                continue;
            }
            if (json[pos] == json[start]) bracket_count++;
            else if (json[pos] == target_bracket) bracket_count--;
            if (bracket_count == 0) break;
        }
        value = json.substr(start, pos - start + 1);
    } else {
        size_t end = json.find_first_of(",}", pos);
        if (end == std::string::npos) end = json.size();
        value = json.substr(pos, end - pos);
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
    }
    
    return value;
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
    : db_(db), total_requests_(0), success_requests_(0), failed_requests_(0) {
    // 初始化默认配置
    config_.api_url = "";
    config_.api_key = "";
    config_.model = "gpt-3.5-turbo";
    config_.timeout_seconds = DEFAULT_TIMEOUT;
    config_.connect_timeout = DEFAULT_CONNECT_TIMEOUT;
    config_.max_retries = DEFAULT_MAX_RETRIES;
    config_.max_message_length = DEFAULT_MAX_MESSAGE_LENGTH;
    config_.enable_cache = true;
}

/**
 * @brief 析构函数
 */
AiService::~AiService() {
    // 清理资源
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
    
    // 从配置文件加载AI配置
    Config& config = Config::getInstance();
    const AIConfig& ai_config = config.getAIConfig();
    
    config_.api_url = ai_config.api_url;
    config_.api_key = ai_config.api_key;
    config_.model = ai_config.model;
    
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
    
    // 异步处理请求
    std::thread([this, request]() {
        asyncProcessRequest(request);
    }).detach();
    
    total_requests_++;
    
    std::cout << "AI request queued: " << request_id << " from " << user_id << std::endl;
    return request_id;
}

/**
 * @brief 同步调用AI API
 */
bool AiService::callAIAPI(const std::string& question, AITone tone, AIPriority priority, 
                           std::string& response) {
    if (config_.api_url.empty() || config_.api_key.empty()) {
        std::cerr << "[AI] API not configured" << std::endl;
        response = "AI服务未配置，请联系管理员";
        return false;
    }
    
    std::cout << "[AI] Calling API: " << config_.api_url << std::endl;
    std::cout << "[AI] Model: " << config_.model << std::endl;
    
    // 检查缓存
    if (config_.enable_cache) {
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
    
    // 构建JSON请求体
    std::ostringstream body;
    body << "{"
         << "\"model\":\"" << config_.model << "\","
         << "\"messages\":["
         << "{\"role\":\"system\",\"content\":\"" << escapeJson(system_prompt) << "\"},"
         << "{\"role\":\"user\",\"content\":\"" << escapeJson(question) << "\"}"
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
 * @brief 拆分AI回复
 */
std::vector<std::string> AiService::splitResponse(const std::string& response, int max_length) {
    std::vector<std::string> messages;
    
    if (response.empty()) {
        return messages;
    }
    
    // 如果回复长度小于等于最大长度，直接返回
    if (response.length() <= static_cast<size_t>(max_length)) {
        messages.push_back(response);
        return messages;
    }
    
    // 中文标点符号（UTF-8编码）
    const std::string period = "。";
    const std::string comma = "，";
    const std::string exclamation = "！";
    const std::string question_mark = "？";
    
    // 按句子拆分
    std::string current_message;
    std::string remaining = response;
    
    while (!remaining.empty()) {
        std::string sentence;
        size_t pos = std::string::npos;
        
        // 查找最近的句号、感叹号、问号
        size_t period_pos = remaining.find(period);
        size_t exclamation_pos = remaining.find(exclamation);
        size_t question_pos = remaining.find(question_mark);
        
        // 找到最近的分隔符
        pos = period_pos;
        if (pos == std::string::npos || (exclamation_pos != std::string::npos && exclamation_pos < pos)) {
            pos = exclamation_pos;
        }
        if (pos == std::string::npos || (question_pos != std::string::npos && question_pos < pos)) {
            pos = question_pos;
        }
        
        if (pos == std::string::npos) {
            // 没有找到分隔符，使用剩余部分
            sentence = remaining;
            remaining.clear();
        } else {
            // 提取句子（包含分隔符，UTF-8中文标点占3字节）
            sentence = remaining.substr(0, pos + 3);
            remaining = remaining.substr(pos + 3);
        }
        
        // 去除前导空格
        size_t start = sentence.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            sentence = sentence.substr(start);
        }
        
        if (sentence.empty()) continue;
        
        // 如果当前消息加上新句子超过最大长度
        if (current_message.length() + sentence.length() > static_cast<size_t>(max_length)) {
            if (!current_message.empty()) {
                messages.push_back(current_message);
                current_message.clear();
            }
            
            // 如果单个句子超过最大长度，按逗号拆分
            if (sentence.length() > static_cast<size_t>(max_length)) {
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
                    
                    if (current_message.length() + sub_sentence.length() > static_cast<size_t>(max_length)) {
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
    
    // 添加最后一条消息
    if (!current_message.empty()) {
        messages.push_back(current_message);
    }
    
    // 如果没有拆分成功，按字数强制拆分
    if (messages.empty()) {
        size_t pos = 0;
        while (pos < response.length()) {
            size_t len = std::min(static_cast<size_t>(max_length), response.length() - pos);
            messages.push_back(response.substr(pos, len));
            pos += len;
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
    snprintf(sql, sizeof(sql),
             "SELECT response FROM ai_cache WHERE question='%s' "
             "AND create_time > DATE_SUB(NOW(), INTERVAL %d HOUR) "
             "ORDER BY create_time DESC LIMIT 1",
             question.c_str(), CACHE_EXPIRE_HOURS);
    
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
void AiService::sendAIResponse(const std::string& user_id, const std::string& target_id, 
                                const std::vector<std::string>& messages, bool is_group,
                                const std::string& origin_user_id) {
    std::cout << "[AI] Sending AI response to " << target_id << ", messages=" << messages.size() << std::endl;
    ChatService& chat_service = ChatService::getInstance();

    // 获取数字ID（用于保存聊天记录）
    uint64_t user_id_num = getUserIdNum(db_, user_id);
    uint64_t target_id_num = getUserIdNum(db_, target_id);

    for (const auto& message : messages) {
        // 构建AI回复消息
        std::ostringstream oss;
        oss << "{\"type\":" << (is_group ? 34 : 40)  // GROUP_MESSAGE or CHAT_PRIVATE
            << ",\"from_user_id\":\"" << user_id << "\""
            << ",\"to_user_id\":\"" << origin_user_id << "\""
            << ",\"content\":\"" << escapeJson(message) << "\""
            << ",\"extra\":\"{\\\"is_ai\\\":true}\""
            << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";
        std::cout << "[AI] Constructed message: " << oss.str() << std::endl;

        std::string msg_str = oss.str();

        // 将 AI 原始消息包装为统一响应格式（与 ChatService::buildResponse 相同）
        std::ostringstream wrapper;
        wrapper << "{"
                << "\"type\":" << (is_group ? 34 : 40)
                << ",\"code\":0,\"msg\":\"\""
                << ",\"data\":" << msg_str
                << ",\"timestamp\":\"" << util::getCurrentTime() << "\"} ";

        std::string notification = wrapper.str();

        if (is_group) {
            chat_service.broadcastToGroup(target_id, notification);
        } else {
            std::cout << "[AI] Sending response to origin user " << origin_user_id << std::endl;
            chat_service.broadcastToUser(origin_user_id, notification);
        }

        // 保存到聊天记录（使用数字ID）
        if (user_id_num > 0 && target_id_num > 0) {
            ChatRecord record;
            record.sender_id = user_id_num;
            record.receiver_id = target_id_num;
            record.group_id = 0;
            record.content = message;
            record.is_ai = 1;

            ChatRecordDAO record_dao(db_);
            record_dao.insert(record);
        }

        // 添加延迟，模拟逐条发送
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

/**
 * @brief 异步处理AI请求
 */
void AiService::asyncProcessRequest(const AIRequest& request) {
    std::cout << "[AI] Processing request: " << request.request_id << std::endl;
    auto start_time = std::chrono::steady_clock::now();
    
    // 获取聊天AI设置（从请求的extra中解析chatKey）
    std::string chat_key = getJsonValue(request.extra, "chatKey");
    if (chat_key.empty()) {
        // 根据目标ID生成chatKey
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
        // 如果没有设置，使用默认值
        ai_nickname = "AI助手";
        std::cout << "[AI] Chat settings not found, using default nickname: " << ai_nickname << std::endl;
    } else {
        std::cout << "[AI] Chat settings: chat_key=" << chat_key << ", nickname=" << ai_nickname 
                  << ", tone=" << tone << ", priority=" << priority << std::endl;
    }
    
    // 调用AI API
    std::string response;
    std::cout << "[AI] Calling API..." << std::endl;
    bool success = callAIAPI(request.question, static_cast<AITone>(tone), 
                            static_cast<AIPriority>(priority), response);
    
    auto end_time = std::chrono::steady_clock::now();
    int response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // 记录日志
    std::cout << "[AI] Logging API call..." << std::endl;
    logAICall(request.request_id, request.user_id, request.question, response, 
              success, success ? "" : response, response_time);
    
    std::cout << "[AI] Success=" << success << ", response length=" << response.length() << std::endl;
    
    if (success) {
        // 拆分回复
        std::cout << "[AI] Splitting response..." << std::endl;
        std::vector<std::string> messages = splitResponse(response, config_.max_message_length);
        std::cout << "[AI] Split into " << messages.size() << " messages" << std::endl;
        
        // 发送回复
        std::cout << "[AI] Sending response to user: " << request.target_id << std::endl;
        sendAIResponse(ai_nickname, request.target_id, messages, request.is_group, request.user_id);
        std::cout << "[AI] Response sent successfully" << std::endl;
    } else {
        // 发送错误消息
        std::cout << "[AI] Sending error message..." << std::endl;
        std::vector<std::string> error_messages = {response};
        sendAIResponse(ai_nickname, request.target_id, error_messages, request.is_group, request.user_id);
    }
    
    // 从待处理队列移除
    std::cout << "[AI] Removing from pending queue..." << std::endl;
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        pending_requests_.erase(request.request_id);
    }
    
    std::cout << "[AI] Request processed: " << request.request_id 
              << " in " << response_time << "ms" << std::endl;
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
