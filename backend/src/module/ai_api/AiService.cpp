/**
 * @file AiService.cpp
 * @brief AI服务模块实现
 */

#include "ai_api/AiService.h"
#include "module/Database.h"
#include "module/Config.h"
#include "module/ChatService.h"
#include "model/ChatRecord.h"
#include "model/ChatRecordDAO.h"
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
        pos++;
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        value = json.substr(pos, end - pos);
    } else if (json[pos] == '{' || json[pos] == '[') {
        int bracket_count = 0;
        char target_bracket = (json[pos] == '{') ? '}' : ']';
        size_t start = pos;
        for (; pos < json.size(); pos++) {
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
    
    std::cout << "AiService initialized successfully" << std::endl;
    std::cout << "AI API URL: " << (config_.api_url.empty() ? "Not configured" : "Configured") << std::endl;
    
    return true;
}

/**
 * @brief 处理AI请求
 */
std::string AiService::processRequest(const std::string& user_id, const std::string& target_id, 
                                       const std::string& question, bool is_group) {
    std::string request_id = generateRequestId();
    
    // 创建请求对象
    AIRequest request;
    request.request_id = request_id;
    request.user_id = user_id;
    request.target_id = target_id;
    request.question = question;
    request.is_group = is_group;
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
        std::cerr << "AI API not configured" << std::endl;
        response = "AI服务未配置，请联系管理员";
        return false;
    }
    
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
    
    std::string system_prompt = "你是一个智能助手。" + tone_prompt + priority_prompt + 
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
    
    for (int retry = 0; retry < config_.max_retries; retry++) {
        if (httpPost(config_.api_url, headers, body.str(), api_response, config_.timeout_seconds)) {
            // 解析响应
            std::string content;
            if (parseAIResponse(api_response, content)) {
                response = content;
                success = true;
                
                // 缓存响应
                if (config_.enable_cache) {
                    cacheResponse(question, response);
                }
                
                break;
            }
        }
        
        std::cerr << "AI API call failed, retry " << (retry + 1) << "/" << config_.max_retries << std::endl;
        
        // 重试前等待
        if (retry < config_.max_retries - 1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500 * (retry + 1)));
        }
    }
    
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
 * @brief 更新用户AI设置
 */
bool AiService::updateUserAISettings(const std::string& user_id, const std::string& nickname, 
                                      int tone, int priority) {
    char sql[1024];
    snprintf(sql, sizeof(sql),
             "UPDATE user SET ai_nickname='%s', ai_tone=%d, ai_priority=%d "
             "WHERE user_id='%s'",
             nickname.c_str(), tone, priority, user_id.c_str());
    
    return db_.execute(sql);
}

/**
 * @brief 获取用户AI设置
 */
bool AiService::getUserAISettings(const std::string& user_id, std::string& nickname, 
                                   int& tone, int& priority) {
    char sql[512];
    snprintf(sql, sizeof(sql),
             "SELECT ai_nickname, ai_tone, ai_priority FROM user WHERE user_id='%s'",
             user_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) {
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return false;
    }
    
    nickname = row[0] ? row[0] : "AI助手";
    tone = row[1] ? atoi(row[1]) : 0;
    priority = row[2] ? atoi(row[2]) : 0;
    
    db_.freeResult(res);
    return true;
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
    
    auto it = response_cache_.find(question);
    if (it != response_cache_.end()) {
        // 检查是否过期
        std::time_t now = std::time(nullptr);
        if (now - it->second.second < CACHE_EXPIRE_HOURS * 3600) {
            response = it->second.first;
            return true;
        }
    }
    
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
    
    // 保存到数据库
    char sql[8192];
    snprintf(sql, sizeof(sql),
             "INSERT INTO ai_cache (question, response, create_time) VALUES ('%s', '%s', NOW())",
             escapeJson(question).c_str(), escapeJson(response).c_str());
    
    return db_.execute(sql);
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
    char sql[8192];
    snprintf(sql, sizeof(sql),
             "INSERT INTO ai_log (request_id, user_id, question, response, success, error_msg, response_time, create_time) "
             "VALUES ('%s', '%s', '%s', '%s', %d, '%s', %d, NOW())",
             request_id.c_str(), user_id.c_str(), 
             escapeJson(question).c_str(), escapeJson(response).c_str(),
             success ? 1 : 0, error_msg.c_str(), response_time);
    
    db_.execute(sql);
}

/**
 * @brief 发送AI回复给用户
 */
void AiService::sendAIResponse(const std::string& user_id, const std::string& target_id, 
                                const std::vector<std::string>& messages, bool is_group) {
    ChatService& chat_service = ChatService::getInstance();
    
    for (const auto& message : messages) {
        // 构建AI回复消息
        std::ostringstream oss;
        oss << "{\"type\":" << (is_group ? 34 : 40)  // GROUP_MESSAGE or CHAT_PRIVATE
            << ",\"from_user_id\":\"" << user_id << "\""
            << ",\"to_user_id\":\"" << target_id << "\""
            << ",\"content\":\"" << escapeJson(message) << "\""
            << ",\"extra\":{\"is_ai\":true}"
            << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";
        
        std::string msg_str = oss.str();
        
        if (is_group) {
            // 群聊消息广播
            chat_service.broadcastToGroup(target_id, msg_str);
        } else {
            // 私聊消息推送
            chat_service.broadcastToUser(target_id, msg_str);
        }
        
        // 保存到聊天记录
        ChatRecord record;
        record.sender_id = strtoull(user_id.c_str(), nullptr, 10);
        record.receiver_id = strtoull(target_id.c_str(), nullptr, 10);
        record.group_id = is_group ? strtoull(target_id.c_str(), nullptr, 10) : 0;
        record.content = message;
        record.is_ai = 1;
        
        ChatRecordDAO record_dao(db_);
        record_dao.insert(record);
        
        // 添加延迟，模拟逐条发送
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

/**
 * @brief 异步处理AI请求
 */
void AiService::asyncProcessRequest(const AIRequest& request) {
    auto start_time = std::chrono::steady_clock::now();
    
    // 获取用户AI设置
    std::string ai_nickname;
    int tone = 0;
    int priority = 0;
    getUserAISettings(request.user_id, ai_nickname, tone, priority);
    
    // 调用AI API
    std::string response;
    bool success = callAIAPI(request.question, static_cast<AITone>(tone), 
                            static_cast<AIPriority>(priority), response);
    
    auto end_time = std::chrono::steady_clock::now();
    int response_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    
    // 记录日志
    logAICall(request.request_id, request.user_id, request.question, response, 
              success, success ? "" : response, response_time);
    
    if (success) {
        // 拆分回复
        std::vector<std::string> messages = splitResponse(response, config_.max_message_length);
        
        // 发送回复
        sendAIResponse(ai_nickname, request.target_id, messages, request.is_group);
    } else {
        // 发送错误消息
        std::vector<std::string> error_messages = {response};
        sendAIResponse(ai_nickname, request.target_id, error_messages, request.is_group);
    }
    
    // 从待处理队列移除
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        pending_requests_.erase(request.request_id);
    }
    
    std::cout << "AI request processed: " << request.request_id 
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
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    
    // 设置请求头
    struct curl_slist* chunk = nullptr;
    for (const auto& header : headers) {
        std::string header_str = header.first + ": " + header.second;
        chunk = curl_slist_append(chunk, header_str.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
    
    CURLcode res = curl_easy_perform(curl);
    
    curl_slist_free_all(chunk);
    curl_easy_cleanup(curl);
    
    return (res == CURLE_OK);
#else
    // 简化实现：使用系统命令调用curl
    std::string temp_file = "/tmp/ai_response_" + std::to_string(std::time(nullptr)) + ".json";
    
    std::ostringstream cmd;
    cmd << "curl -s -X POST " << url
        << " -H 'Content-Type: application/json'";
    
    for (const auto& header : headers) {
        if (header.first != "Content-Type") {
            cmd << " -H '" << header.first << ": " << header.second << "'";
        }
    }
    
    cmd << " -d '" << body << "'"
        << " -o " << temp_file
        << " --connect-timeout " << timeout;
    
    int ret = system(cmd.str().c_str());
    
    if (ret == 0) {
        std::ifstream file(temp_file);
        if (file.is_open()) {
            std::ostringstream oss;
            oss << file.rdbuf();
            response = oss.str();
            file.close();
            
            // 删除临时文件
            std::remove(temp_file.c_str());
            return true;
        }
    }
    
    return false;
#endif
}

/**
 * @brief 解析AI API响应
 */
bool AiService::parseAIResponse(const std::string& response, std::string& content) {
    try {
        // 解析OpenAI格式的响应
        // {"choices":[{"message":{"content":"..."}}]}
        
        std::string choices = getJsonValue(response, "choices");
        if (choices.empty()) {
            // 尝试解析其他格式
            content = getJsonValue(response, "content");
            if (content.empty()) {
                content = getJsonValue(response, "response");
                if (content.empty()) {
                    content = getJsonValue(response, "text");
                }
            }
            return !content.empty();
        }
        
        // 解析choices数组
        if (choices[0] == '[') {
            // 找到第一个message对象
            size_t msg_pos = choices.find("\"message\"");
            if (msg_pos != std::string::npos) {
                std::string message = choices.substr(msg_pos);
                content = getJsonValue(message, "content");
                return !content.empty();
            }
        }
        
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Parse AI response error: " << e.what() << std::endl;
        return false;
    }
}
