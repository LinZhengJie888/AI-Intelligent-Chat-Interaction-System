/**
 * @file ChatService.cpp
 * @brief 消息路由中心模块实现
 */

#include "module/ChatService.h"
#include "verify/VerifyService.h"
#include "friend/FriendService.h"
#include "friend/GroupService.h"
#include "ai_api/AiService.h"
#include "module/Database.h"
#include "module/Config.h"
#include "model/User.h"
#include "model/UserDAO.h"
#include "model/ChatRecord.h"
#include "model/ChatRecordDAO.h"
#include "model/GroupMember.h"
#include "common/Util.h"
#include <iostream>
#include <sstream>
#include <algorithm>

/**
 * @brief JSON解析辅助函数（简易实现）
 * @param json JSON字符串
 * @param key 键名
 * @return 键值字符串
 */
static std::string getJsonValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return "";
    
    pos = json.find(":", pos);
    if (pos == std::string::npos) return "";
    pos++;
    
    // 跳过空白
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.size()) return "";
    
    std::string value;
    if (json[pos] == '"') {
        // 字符串值
        pos++;
        size_t end = json.find("\"", pos);
        if (end == std::string::npos) return "";
        value = json.substr(pos, end - pos);
    } else if (json[pos] == '{' || json[pos] == '[') {
        // 对象或数组
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
        // 数字或布尔值
        size_t end = json.find_first_of(",}", pos);
        if (end == std::string::npos) end = json.size();
        value = json.substr(pos, end - pos);
        // 去除空白
        value.erase(std::remove_if(value.begin(), value.end(), ::isspace), value.end());
    }
    
    return value;
}

/**
 * @brief JSON转义辅助函数
 * @param str 输入字符串
 * @return 转义后的字符串
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
 * @brief 获取单例实例
 */
ChatService& ChatService::getInstance() {
    static ChatService instance;
    return instance;
}

/**
 * @brief 私有构造函数
 */
ChatService::ChatService() : db_(nullptr), initialized_(false) {}

/**
 * @brief 析构函数
 */
ChatService::~ChatService() {
    // 清理资源
}

/**
 * @brief 初始化服务
 */
bool ChatService::init(Database& db) {
    if (initialized_) return true;
    
    db_ = &db;
    
    // 初始化各业务模块
    verify_service_ = std::unique_ptr<VerifyService>(new VerifyService(db));
    friend_service_ = std::unique_ptr<FriendService>(new FriendService(db));
    group_service_ = std::unique_ptr<GroupService>(new GroupService(db));
    ai_service_ = std::unique_ptr<AiService>(new AiService(db));
    
    if (!verify_service_->init()) {
        std::cerr << "VerifyService init failed" << std::endl;
        return false;
    }
    
    if (!friend_service_->init()) {
        std::cerr << "FriendService init failed" << std::endl;
        return false;
    }
    
    if (!group_service_->init()) {
        std::cerr << "GroupService init failed" << std::endl;
        return false;
    }
    
    if (!ai_service_->init()) {
        std::cerr << "AiService init failed" << std::endl;
        return false;
    }
    
    initialized_ = true;
    std::cout << "ChatService initialized successfully" << std::endl;
    return true;
}

/**
 * @brief 处理新连接
 */
void ChatService::handleNewConnection(spConnection conn) {
    std::cout << "New connection: fd=" << conn->fd() 
              << ", ip=" << conn->ip() 
              << ", port=" << conn->port() << std::endl;
}

/**
 * @brief 处理连接关闭
 */
void ChatService::handleClose(spConnection conn) {
    int fd = conn->fd();
    
    std::lock_guard<std::mutex> lock(conn_mutex_);
    
    // 查找并移除用户连接映射
    auto it = fd_to_user_.find(fd);
    if (it != fd_to_user_.end()) {
        std::string user_id = it->second;
        user_connections_.erase(user_id);
        fd_to_user_.erase(it);
        std::cout << "User " << user_id << " disconnected" << std::endl;
    }
}

/**
 * @brief 处理消息
 */
void ChatService::handleMessage(spConnection conn, std::string& message) {
    if (!initialized_) {
        sendResponse(conn, 0, -1, "Service not initialized");
        return;
    }
    
    Message msg;
    if (!parseMessage(message, msg)) {
        sendResponse(conn, 0, -1, "Invalid message format");
        return;
    }
    
    // 根据消息类型分发处理
    switch (static_cast<MessageType>(msg.type)) {
        case MessageType::LOGIN:
            handleLogin(conn, msg);
            break;
        case MessageType::REGISTER:
            handleRegister(conn, msg);
            break;
        case MessageType::LOGOUT:
            handleLogout(conn, msg);
            break;
        case MessageType::GET_CAPTCHA:
            handleGetCaptcha(conn, msg);
            break;
        case MessageType::FRIEND_ADD:
            handleFriendAdd(conn, msg);
            break;
        case MessageType::FRIEND_AGREE:
            handleFriendAgree(conn, msg);
            break;
        case MessageType::FRIEND_REJECT:
            handleFriendReject(conn, msg);
            break;
        case MessageType::FRIEND_LIST:
            handleFriendList(conn, msg);
            break;
        case MessageType::FRIEND_DELETE:
            handleFriendDelete(conn, msg);
            break;
        case MessageType::GROUP_CREATE:
            handleGroupCreate(conn, msg);
            break;
        case MessageType::GROUP_JOIN:
            handleGroupJoin(conn, msg);
            break;
        case MessageType::GROUP_AGREE:
            handleGroupAgree(conn, msg);
            break;
        case MessageType::GROUP_REJECT:
            handleGroupReject(conn, msg);
            break;
        case MessageType::GROUP_MESSAGE:
            handleGroupMessage(conn, msg);
            break;
        case MessageType::GROUP_MEMBERS:
            handleGroupMembers(conn, msg);
            break;
        case MessageType::GROUP_LIST:
            handleGroupList(conn, msg);
            break;
        case MessageType::CHAT_PRIVATE:
            handlePrivateChat(conn, msg);
            break;
        case MessageType::CHAT_HISTORY:
            handleChatHistory(conn, msg);
            break;
        case MessageType::AI_REQUEST:
            handleAiRequest(conn, msg);
            break;
        case MessageType::AI_AT:
            handleAiAt(conn, msg);
            break;
        case MessageType::AI_SETTING:
            handleAiSetting(conn, msg);
            break;
        default:
            sendResponse(conn, msg.type, -1, "Unknown message type");
            break;
    }
}

/**
 * @brief 处理发送完成
 */
void ChatService::handleSendComplete(spConnection conn) {
    // 发送完成处理（可选）
}

/**
 * @brief 处理超时
 */
void ChatService::handleTimeout(spConnection conn) {
    // 超时处理（可选）
}

/**
 * @brief 解析消息JSON
 */
bool ChatService::parseMessage(const std::string& json_str, Message& msg) {
    try {
        std::string type_str = getJsonValue(json_str, "type");
        if (type_str.empty()) return false;
        msg.type = std::stoi(type_str);
        
        msg.from_user_id = getJsonValue(json_str, "from_user_id");
        msg.to_user_id = getJsonValue(json_str, "to_user_id");
        msg.content = getJsonValue(json_str, "content");
        msg.extra = getJsonValue(json_str, "extra");
        msg.timestamp = getJsonValue(json_str, "timestamp");
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Parse message error: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 构建响应JSON
 */
std::string ChatService::buildResponse(int type, int code, const std::string& msg, 
                                       const std::string& data) {
    std::ostringstream oss;
    oss << "{\"type\":" << type
        << ",\"code\":" << code
        << ",\"msg\":\"" << escapeJson(msg) << "\"";
    
    if (!data.empty()) {
        oss << ",\"data\":" << data;
    }
    
    oss << ",\"timestamp\":\"" << util::getCurrentTime() << "\"}";
    return oss.str();
}

/**
 * @brief 发送响应给客户端
 */
void ChatService::sendResponse(spConnection conn, int type, int code, 
                               const std::string& msg, const std::string& data) {
    std::string response = buildResponse(type, code, msg, data);
    conn->send(response.data(), response.size());
}

/**
 * @brief 处理登录请求
 */
void ChatService::handleLogin(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string password = getJsonValue(msg.extra, "password");
    std::string captcha = getJsonValue(msg.extra, "captcha");
    std::string captcha_token = getJsonValue(msg.extra, "captcha_token");
    
    if (user_id.empty() || password.empty() || captcha.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::LOGIN), -1, 
                    "Missing required fields");
        return;
    }
    
    // 验证验证码
    if (!verify_service_->verifyCaptcha(captcha_token, captcha)) {
        sendResponse(conn, static_cast<int>(MessageType::LOGIN), -1, 
                    "Invalid captcha");
        return;
    }
    
    // 验证密码
    UserDAO user_dao(*db_);
    if (!user_dao.verifyPassword(user_id, password)) {
        sendResponse(conn, static_cast<int>(MessageType::LOGIN), -1, 
                    "Invalid user_id or password");
        return;
    }
    
    // 查询用户信息
    User* user = user_dao.findByUserId(user_id);
    if (!user) {
        sendResponse(conn, static_cast<int>(MessageType::LOGIN), -1, 
                    "User not found");
        return;
    }
    
    // 记录用户连接
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        user_connections_[user_id] = conn;
        fd_to_user_[conn->fd()] = user_id;
    }
    
    // 构建用户信息响应
    std::ostringstream user_data;
    user_data << "{\"user_id\":\"" << user->user_id << "\""
              << ",\"username\":\"" << escapeJson(user->username) << "\""
              << ",\"nickname\":\"" << escapeJson(user->nickname) << "\""
              << ",\"avatar_path\":\"" << escapeJson(user->avatar_path) << "\""
              << ",\"ai_nickname\":\"" << escapeJson(user->ai_nickname) << "\""
              << ",\"ai_tone\":" << static_cast<int>(user->ai_tone)
              << ",\"ai_priority\":" << static_cast<int>(user->ai_priority)
              << "}";
    
    sendResponse(conn, static_cast<int>(MessageType::LOGIN), 0, 
                "Login successful", user_data.str());
    
    std::cout << "User " << user_id << " logged in" << std::endl;
    delete user;
}

/**
 * @brief 处理注册请求
 */
void ChatService::handleRegister(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string username = getJsonValue(msg.extra, "username");
    std::string password = getJsonValue(msg.extra, "password");
    std::string captcha = getJsonValue(msg.extra, "captcha");
    std::string captcha_token = getJsonValue(msg.extra, "captcha_token");
    
    if (user_id.empty() || username.empty() || password.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::REGISTER), -1, 
                    "Missing required fields");
        return;
    }
    
    // 验证验证码（注册也需要验证码）
    if (!captcha.empty() && !captcha_token.empty()) {
        if (!verify_service_->verifyCaptcha(captcha_token, captcha)) {
            sendResponse(conn, static_cast<int>(MessageType::REGISTER), -1, 
                        "Invalid captcha");
            return;
        }
    }
    
    // 检查用户ID是否已存在
    UserDAO user_dao(*db_);
    User* existing = user_dao.findByUserId(user_id);
    if (existing) {
        delete existing;
        sendResponse(conn, static_cast<int>(MessageType::REGISTER), -1, 
                    "User ID already exists");
        return;
    }
    
    // 创建新用户
    User new_user;
    new_user.user_id = user_id;
    new_user.username = username;
    new_user.nickname = username;  // 默认昵称为用户名
    new_user.password = util::md5(password);
    new_user.phone = "";
    new_user.avatar_path = "";
    new_user.ai_nickname = Config::getInstance().getAIConfig().default_nickname;
    new_user.ai_tone = Config::getInstance().getAIConfig().default_tone;
    new_user.ai_priority = Config::getInstance().getAIConfig().default_priority;
    
    if (!user_dao.insert(new_user)) {
        sendResponse(conn, static_cast<int>(MessageType::REGISTER), -1, 
                    "Registration failed");
        return;
    }
    
    sendResponse(conn, static_cast<int>(MessageType::REGISTER), 0, 
                "Registration successful");
    
    std::cout << "User " << user_id << " registered" << std::endl;
}

/**
 * @brief 处理登出请求
 */
void ChatService::handleLogout(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        user_connections_.erase(user_id);
        fd_to_user_.erase(conn->fd());
    }
    
    sendResponse(conn, static_cast<int>(MessageType::LOGOUT), 0, 
                "Logout successful");
    
    std::cout << "User " << user_id << " logged out" << std::endl;
}

/**
 * @brief 处理获取验证码请求
 */
void ChatService::handleGetCaptcha(spConnection conn, const Message& msg) {
    std::string phone = getJsonValue(msg.extra, "phone");
    
    // 生成验证码
    std::string captcha_token;
    std::string captcha_image;  // Base64编码的验证码图片
    
    if (!verify_service_->generateCaptcha(phone, captcha_token, captcha_image)) {
        sendResponse(conn, static_cast<int>(MessageType::GET_CAPTCHA), -1, 
                    "Failed to generate captcha");
        return;
    }
    
    std::string data = "{\"captcha_token\":\"" + captcha_token + 
                       "\",\"captcha_image\":\"" + captcha_image + "\"}";
    
    sendResponse(conn, static_cast<int>(MessageType::GET_CAPTCHA), 0, 
                "Captcha generated", data);
}

/**
 * @brief 处理添加好友请求
 */
void ChatService::handleFriendAdd(spConnection conn, const Message& msg) {
    std::string from_user_id = msg.from_user_id;
    std::string to_user_id = msg.to_user_id;
    std::string request_msg = msg.content;
    
    if (from_user_id.empty() || to_user_id.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_ADD), -1, 
                    "Missing required fields");
        return;
    }
    
    int result = friend_service_->sendRequest(from_user_id, to_user_id, request_msg);
    if (result == 0) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_ADD), 0, 
                    "Friend request sent");
        
        // 推送通知给目标用户
        std::string notification = buildResponse(
            static_cast<int>(MessageType::FRIEND_ADD), 0, 
            "You have a new friend request",
            "{\"from_user_id\":\"" + from_user_id + "\"}");
        broadcastToUser(to_user_id, notification);
    } else if (result == -2) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_ADD), -1, 
                    "Already friends");
    } else if (result == -3) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_ADD), -1, 
                    "Request already sent");
    } else if (result == -4) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_ADD), -1, 
                    "In cooling period");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_ADD), -1, 
                    "Failed to send friend request");
    }
}

/**
 * @brief 处理同意好友请求
 */
void ChatService::handleFriendAgree(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string from_user_id = msg.to_user_id;  // 请求方用户ID
    
    if (friend_service_->agreeRequest(from_user_id, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_AGREE), 0, 
                    "Friend request agreed");
        
        // 通知请求方
        std::string notification = buildResponse(
            static_cast<int>(MessageType::FRIEND_AGREE), 0, 
            "Your friend request has been agreed",
            "{\"from_user_id\":\"" + user_id + "\"}");
        broadcastToUser(from_user_id, notification);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_AGREE), -1, 
                    "Failed to agree friend request");
    }
}

/**
 * @brief 处理拒绝好友请求
 */
void ChatService::handleFriendReject(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string from_user_id = msg.to_user_id;  // 请求方用户ID
    
    if (friend_service_->rejectRequest(from_user_id, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_REJECT), 0, 
                    "Friend request rejected");
        
        // 通知请求方
        std::string notification = buildResponse(
            static_cast<int>(MessageType::FRIEND_REJECT), 0, 
            "Your friend request has been rejected",
            "{\"from_user_id\":\"" + user_id + "\"}");
        broadcastToUser(from_user_id, notification);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_REJECT), -1, 
                    "Failed to reject friend request");
    }
}

/**
 * @brief 处理获取好友列表请求
 */
void ChatService::handleFriendList(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    
    std::string friends_json = friend_service_->getFriendList(user_id);
    
    sendResponse(conn, static_cast<int>(MessageType::FRIEND_LIST), 0, 
                "Success", friends_json);
}

/**
 * @brief 处理删除好友请求
 */
void ChatService::handleFriendDelete(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string friend_id = msg.to_user_id;
    
    if (friend_service_->deleteFriend(user_id, friend_id)) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_DELETE), 0, 
                    "Friend deleted");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_DELETE), -1, 
                    "Failed to delete friend");
    }
}

/**
 * @brief 处理创建群聊请求
 */
void ChatService::handleGroupCreate(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_name = getJsonValue(msg.extra, "group_name");
    
    if (group_name.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_CREATE), -1, 
                    "Missing group name");
        return;
    }
    
    std::string group_id;
    if (group_service_->createGroup(user_id, group_name, group_id)) {
        std::string data = "{\"group_id\":\"" + group_id + 
                           "\",\"group_name\":\"" + escapeJson(group_name) + "\"}";
        sendResponse(conn, static_cast<int>(MessageType::GROUP_CREATE), 0, 
                    "Group created", data);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_CREATE), -1, 
                    "Failed to create group");
    }
}

/**
 * @brief 处理加群申请请求
 */
void ChatService::handleGroupJoin(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    std::string request_msg = msg.content;
    
    int result = group_service_->sendJoinRequest(user_id, group_id, request_msg);
    if (result == 0) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_JOIN), 0, 
                    "Join request sent");
        
        // 通知群主
        std::string creator_id = group_service_->getGroupCreator(group_id);
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_JOIN), 0, 
            "You have a new group join request",
            "{\"from_user_id\":\"" + user_id + "\",\"group_id\":\"" + group_id + "\"}");
        broadcastToUser(creator_id, notification);
    } else if (result == -2) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_JOIN), -1, 
                    "Already a member");
    } else if (result == -3) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_JOIN), -1, 
                    "Request already sent");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_JOIN), -1, 
                    "Failed to send join request");
    }
}

/**
 * @brief 处理同意加群申请请求
 */
void ChatService::handleGroupAgree(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;  // 群主ID
    std::string from_user_id = msg.to_user_id;  // 申请人ID
    std::string group_id = getJsonValue(msg.extra, "group_id");
    
    if (group_service_->agreeJoinRequest(from_user_id, group_id, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_AGREE), 0, 
                    "Join request agreed");
        
        // 通知申请人
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_AGREE), 0, 
            "Your group join request has been agreed",
            "{\"group_id\":\"" + group_id + "\"}");
        broadcastToUser(from_user_id, notification);
        
        // 通知群成员
        std::string member_notification = buildResponse(
            static_cast<int>(MessageType::GROUP_AGREE), 0, 
            "New member joined the group",
            "{\"user_id\":\"" + from_user_id + "\",\"group_id\":\"" + group_id + "\"}");
        broadcastToGroup(group_id, member_notification, from_user_id);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_AGREE), -1, 
                    "Failed to agree join request");
    }
}

/**
 * @brief 处理拒绝加群申请请求
 */
void ChatService::handleGroupReject(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;  // 群主ID
    std::string from_user_id = msg.to_user_id;  // 申请人ID
    std::string group_id = getJsonValue(msg.extra, "group_id");
    
    if (group_service_->rejectJoinRequest(from_user_id, group_id, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_REJECT), 0, 
                    "Join request rejected");
        
        // 通知申请人
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_REJECT), 0, 
            "Your group join request has been rejected",
            "{\"group_id\":\"" + group_id + "\"}");
        broadcastToUser(from_user_id, notification);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_REJECT), -1, 
                    "Failed to reject join request");
    }
}

/**
 * @brief 处理群聊消息请求
 */
void ChatService::handleGroupMessage(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    std::string content = msg.content;
    
    // 检查是否是AI请求（@召唤）
    std::string ai_nickname = getJsonValue(msg.extra, "ai_nickname");
    if (!ai_nickname.empty() && content.find("@" + ai_nickname) == 0) {
        // 提取AI问题
        std::string question = content.substr(ai_nickname.length() + 1);
        // 去除前导空格
        size_t start = question.find_first_not_of(" ");
        if (start != std::string::npos) {
            question = question.substr(start);
        }
        
        // 异步处理AI请求
        ai_service_->processRequest(user_id, group_id, question, true);
        
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MESSAGE), 0, 
                    "Message sent");
        return;
    }
    
    // 保存消息并广播
    if (group_service_->sendMessage(user_id, group_id, content)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MESSAGE), 0, 
                    "Message sent");
        
        // 广播给群成员
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_MESSAGE), 0, "",
            "{\"from_user_id\":\"" + user_id + "\",\"group_id\":\"" + group_id + 
            "\",\"content\":\"" + escapeJson(content) + "\"}");
        broadcastToGroup(group_id, notification, user_id);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MESSAGE), -1, 
                    "Failed to send message");
    }
}

/**
 * @brief 处理获取群成员列表请求
 */
void ChatService::handleGroupMembers(spConnection conn, const Message& msg) {
    std::string group_id = msg.to_user_id;
    
    std::string members_json = group_service_->getGroupMembers(group_id);
    
    sendResponse(conn, static_cast<int>(MessageType::GROUP_MEMBERS), 0, 
                "Success", members_json);
}

/**
 * @brief 处理获取群列表请求
 */
void ChatService::handleGroupList(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    
    std::string groups_json = group_service_->getUserGroups(user_id);
    
    sendResponse(conn, static_cast<int>(MessageType::GROUP_LIST), 0, 
                "Success", groups_json);
}

/**
 * @brief 处理私聊消息请求
 */
void ChatService::handlePrivateChat(spConnection conn, const Message& msg) {
    std::string from_user_id = msg.from_user_id;
    std::string to_user_id = msg.to_user_id;
    std::string content = msg.content;
    
    // 检查是否是AI请求
    std::string ai_nickname = getJsonValue(msg.extra, "ai_nickname");
    if (!ai_nickname.empty() && content.find("@" + ai_nickname) == 0) {
        std::string question = content.substr(ai_nickname.length() + 1);
        size_t start = question.find_first_not_of(" ");
        if (start != std::string::npos) {
            question = question.substr(start);
        }
        
        ai_service_->processRequest(from_user_id, to_user_id, question, false);
        
        sendResponse(conn, static_cast<int>(MessageType::CHAT_PRIVATE), 0, 
                    "Message sent");
        return;
    }
    
    // 检查是否是好友关系
    if (!friend_service_->isFriend(from_user_id, to_user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::CHAT_PRIVATE), -1, 
                    "Not friends");
        return;
    }
    
    // 保存消息
    ChatRecord record;
    record.sender_id = strtoull(from_user_id.c_str(), nullptr, 10);
    record.receiver_id = strtoull(to_user_id.c_str(), nullptr, 10);
    record.group_id = 0;
    record.content = content;
    record.is_ai = 0;
    
    ChatRecordDAO record_dao(*db_);
    if (record_dao.insert(record)) {
        sendResponse(conn, static_cast<int>(MessageType::CHAT_PRIVATE), 0, 
                    "Message sent");
        
        // 推送给目标用户
        std::string notification = buildResponse(
            static_cast<int>(MessageType::CHAT_PRIVATE), 0, "",
            "{\"from_user_id\":\"" + from_user_id + "\",\"content\":\"" + 
            escapeJson(content) + "\"}");
        broadcastToUser(to_user_id, notification);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::CHAT_PRIVATE), -1, 
                    "Failed to send message");
    }
}

/**
 * @brief 处理获取聊天记录请求
 */
void ChatService::handleChatHistory(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string target_id = msg.to_user_id;
    bool is_group = (getJsonValue(msg.extra, "is_group") == "true");
    
    ChatRecordDAO record_dao(*db_);
    std::vector<ChatRecord> records;
    
    if (is_group) {
        uint64_t group_id = strtoull(target_id.c_str(), nullptr, 10);
        records = record_dao.findByGroup(group_id);
    } else {
        uint64_t user1 = strtoull(user_id.c_str(), nullptr, 10);
        uint64_t user2 = strtoull(target_id.c_str(), nullptr, 10);
        records = record_dao.findByUserPair(user1, user2);
    }
    
    // 构建JSON响应
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < records.size(); i++) {
        if (i > 0) oss << ",";
        oss << "{\"id\":" << records[i].id
            << ",\"sender_id\":" << records[i].sender_id
            << ",\"receiver_id\":" << records[i].receiver_id
            << ",\"group_id\":" << records[i].group_id
            << ",\"content\":\"" << escapeJson(records[i].content) << "\""
            << ",\"send_time\":\"" << records[i].send_time << "\""
            << ",\"is_ai\":" << static_cast<int>(records[i].is_ai)
            << "}";
    }
    oss << "]";
    
    sendResponse(conn, static_cast<int>(MessageType::CHAT_HISTORY), 0, 
                "Success", oss.str());
}

/**
 * @brief 处理AI请求（按键调用）
 */
void ChatService::handleAiRequest(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string target_id = msg.to_user_id;
    std::string question = msg.content;
    bool is_group = (getJsonValue(msg.extra, "is_group") == "true");
    
    ai_service_->processRequest(user_id, target_id, question, is_group);
    
    sendResponse(conn, static_cast<int>(MessageType::AI_REQUEST), 0, 
                "AI request received");
}

/**
 * @brief 处理AI请求（@召唤）
 */
void ChatService::handleAiAt(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string target_id = msg.to_user_id;
    std::string question = msg.content;
    bool is_group = (getJsonValue(msg.extra, "is_group") == "true");
    
    ai_service_->processRequest(user_id, target_id, question, is_group);
    
    sendResponse(conn, static_cast<int>(MessageType::AI_AT), 0, 
                "AI request received");
}

/**
 * @brief 处理AI设置修改请求
 */
void ChatService::handleAiSetting(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string ai_nickname = getJsonValue(msg.extra, "ai_nickname");
    std::string ai_tone_str = getJsonValue(msg.extra, "ai_tone");
    std::string ai_priority_str = getJsonValue(msg.extra, "ai_priority");
    
    UserDAO user_dao(*db_);
    User* user = user_dao.findByUserId(user_id);
    if (!user) {
        sendResponse(conn, static_cast<int>(MessageType::AI_SETTING), -1, 
                    "User not found");
        return;
    }
    
    if (!ai_nickname.empty()) {
        user->ai_nickname = ai_nickname;
    }
    if (!ai_tone_str.empty()) {
        user->ai_tone = std::stoi(ai_tone_str);
    }
    if (!ai_priority_str.empty()) {
        user->ai_priority = std::stoi(ai_priority_str);
    }
    
    if (user_dao.update(*user)) {
        sendResponse(conn, static_cast<int>(MessageType::AI_SETTING), 0, 
                    "AI settings updated");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::AI_SETTING), -1, 
                    "Failed to update AI settings");
    }
    
    delete user;
}

/**
 * @brief 广播消息给指定用户
 */
void ChatService::broadcastToUser(const std::string& user_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    
    auto it = user_connections_.find(user_id);
    if (it != user_connections_.end()) {
        it->second->send(message.data(), message.size());
    }
}

/**
 * @brief 广播消息给群聊所有成员
 */
void ChatService::broadcastToGroup(const std::string& group_id, const std::string& message, 
                                   const std::string& exclude_user_id) {
    std::vector<GroupMember> members = group_service_->getGroupMembersList(group_id);
    
    std::lock_guard<std::mutex> lock(conn_mutex_);
    
    for (const auto& member : members) {
        std::string member_user_id = std::to_string(member.user_id);
        if (member_user_id == exclude_user_id) continue;
        
        auto it = user_connections_.find(member_user_id);
        if (it != user_connections_.end()) {
            it->second->send(message.data(), message.size());
        }
    }
}
