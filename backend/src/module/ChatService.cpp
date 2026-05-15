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
#include "common/JsonUtil.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * @brief JSON解析辅助函数（使用JsonUtil）
 * @param json JSON字符串
 * @param key 键名
 * @return 键值字符串
 */
static std::string getJsonValue(const std::string& json, const std::string& key) {
    return JsonUtil::getString(json, key);
}

/**
 * @brief JSON转义辅助函数
 * @param str 输入字符串
 * @return 转义后的字符串
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
 * @brief 根据数字ID获取用户ID字符串
 * @param db 数据库连接
 * @param id 用户数字ID
 * @return 用户ID字符串，失败返回空字符串
 */
static std::string getUserIdStr(Database& db, uint64_t id) {
    if (id == 0) return "";
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT user_id FROM user WHERE id=%lu", (unsigned long)id);
    
    MYSQL_RES* res = db.query(sql);
    if (!res) return "";
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || !row[0]) {
        db.freeResult(res);
        return "";
    }
    
    std::string user_id = row[0];
    db.freeResult(res);
    return user_id;
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
    std::cout << "[ChatService] Received raw message (len=" << message.size() << "): "
              << (message.size() > 200 ? message.substr(0,200) + "..." : message) << std::endl;
    if (!initialized_) {
        sendResponse(conn, 0, -1, "Service not initialized");
        return;
    }
    
    Message msg;
    if (!parseMessage(message, msg)) {
        sendResponse(conn, 0, -1, "Invalid message format");
        return;
    }

    // 如果消息中没有 from_user_id，尝试从 fd_to_user_ 中恢复
    if (msg.from_user_id.empty()) {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        auto it = fd_to_user_.find(conn->fd());
        if (it != fd_to_user_.end() && !it->second.empty()) {
            msg.from_user_id = it->second;
            // 确保 user_connections_ 映射正确
            user_connections_[msg.from_user_id] = conn;
            std::cout << "[ChatService] Recovered from_user_id=" << msg.from_user_id << " for fd=" << conn->fd() << std::endl;
        }
    } else {
        // 如果消息中有 from_user_id，确保连接映射正确
        std::lock_guard<std::mutex> lock(conn_mutex_);
        auto it = fd_to_user_.find(conn->fd());
        if (it == fd_to_user_.end() || it->second != msg.from_user_id) {
            // 注册新连接
            fd_to_user_[conn->fd()] = msg.from_user_id;
            user_connections_[msg.from_user_id] = conn;
            std::cout << "[ChatService] Mapped fd=" << conn->fd() << " to user_id=" << msg.from_user_id << std::endl;
        }
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
        case MessageType::UPDATE_USERNAME:
            handleUpdateUsername(conn, msg);
            break;
        case MessageType::MESSAGE_RECALL:
            handleMessageRecall(conn, msg);
            break;
        case MessageType::MESSAGE_READ:
            handleMessageRead(conn, msg);
            break;
        case MessageType::FRIEND_REQUEST_LIST:
            handleFriendRequestList(conn, msg);
            break;
        case MessageType::FRIEND_SET_REMARK:
            handleFriendSetRemark(conn, msg);
            break;
        case MessageType::GROUP_REQUEST_LIST:
            handleGroupRequestList(conn, msg);
            break;
        case MessageType::GROUP_MODIFY_NAME:
            handleGroupModifyName(conn, msg);
            break;
        case MessageType::GROUP_LEAVE:
            handleGroupLeave(conn, msg);
            break;
        case MessageType::GROUP_KICK:
            handleGroupKick(conn, msg);
            break;
        case MessageType::GROUP_DISSOLVE:
            handleGroupDissolve(conn, msg);
            break;
        case MessageType::UPLOAD_AVATAR:
            handleUploadAvatar(conn, msg);
            break;
        case MessageType::UPLOAD_GROUP_AVATAR:
            handleUploadGroupAvatar(conn, msg);
            break;
        case static_cast<MessageType>(0):  // 心跳消息
            // 心跳包，更新连接活跃时间，不返回响应
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
    std::cout << "[ChatService] Sending response: " << response << std::endl;
    try {
        conn->send(response.data(), response.size());
    } catch (const std::exception& e) {
        std::cerr << "[ChatService] Failed to send response to fd=" << conn->fd() << ": " << e.what()
                  << ", response=" << response << std::endl;
        // 清理失效连接映射
        std::lock_guard<std::mutex> lock(conn_mutex_);
        auto it = fd_to_user_.find(conn->fd());
        if (it != fd_to_user_.end()) {
            user_connections_.erase(it->second);
            fd_to_user_.erase(it);
        }
    }
}

/**
 * @brief 处理登录请求
 */
void ChatService::handleLogin(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string password = getJsonValue(msg.extra, "password");
    std::string captcha = getJsonValue(msg.extra, "captcha");
    std::string captcha_token = getJsonValue(msg.extra, "captcha_token");
    
    if (user_id.empty() || password.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::LOGIN), -1, 
                    "Missing required fields");
        return;
    }
    
    // 验证验证码（仅在提供验证码时验证，演示模式可跳过）
    if (!captcha.empty()) {
        if (!verify_service_->verifyCaptcha(captcha_token, captcha)) {
            sendResponse(conn, static_cast<int>(MessageType::LOGIN), -1, 
                        "Invalid captcha");
            return;
        }
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
        
        // 通知被删除方
        std::string notification = buildResponse(
            static_cast<int>(MessageType::FRIEND_DELETE), 0, 
            "You have been removed from friend list",
            "{\"from_user_id\":\"" + user_id + "\"}");
        broadcastToUser(friend_id, notification);
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
        
        // 先保存@AI消息到数据库并广播给群成员（让其他人看到谁召唤了AI）
        if (group_service_->sendMessage(user_id, group_id, content)) {
            std::string notification = buildResponse(
                static_cast<int>(MessageType::GROUP_MESSAGE), 0, "",
                "{\"from_user_id\":\"" + user_id + "\",\"group_id\":\"" + group_id + 
                "\",\"content\":\"" + escapeJson(content) + "\"}");
            broadcastToGroup(group_id, notification, user_id);
        }
        
        // 异步处理AI请求
        ai_service_->processRequest(user_id, group_id, question, true, msg.extra);
        
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
        
        // 保存用户消息到聊天记录
        uint64_t from_id_num = getUserIdNum(*db_, from_user_id);
        uint64_t to_id_num = getUserIdNum(*db_, to_user_id);
        if (from_id_num > 0 && to_id_num > 0) {
            ChatRecord record;
            record.sender_id = from_id_num;
            record.receiver_id = to_id_num;
            record.group_id = 0;
            record.content = content;
            record.is_ai = 0;
            ChatRecordDAO record_dao(*db_);
            record_dao.insert(record);
        }
        
        ai_service_->processRequest(from_user_id, to_user_id, question, false, msg.extra);
        
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
    
    // 获取数字ID
    uint64_t from_id_num = getUserIdNum(*db_, from_user_id);
    uint64_t to_id_num = getUserIdNum(*db_, to_user_id);
    if (from_id_num == 0 || to_id_num == 0) {
        sendResponse(conn, static_cast<int>(MessageType::CHAT_PRIVATE), -1, 
                    "User not found");
        return;
    }
    
    // 保存消息
    ChatRecord record;
    record.sender_id = from_id_num;
    record.receiver_id = to_id_num;
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
        // 群聊：使用 group_id 查询
        // 先从 group_chat 表获取数字 ID
        char group_sql[256];
        snprintf(group_sql, sizeof(group_sql), 
                 "SELECT id FROM group_chat WHERE group_id='%s'", 
                 db_->escapeString(target_id).c_str());
        MYSQL_RES* group_res = db_->query(group_sql);
        uint64_t group_id_num = 0;
        if (group_res) {
            MYSQL_ROW group_row = mysql_fetch_row(group_res);
            if (group_row && group_row[0]) {
                group_id_num = strtoull(group_row[0], nullptr, 10);
            }
            db_->freeResult(group_res);
        }
        
        if (group_id_num == 0) {
            std::cerr << "ChatHistory: group not found, group_id=" << target_id << std::endl;
            records = record_dao.findByGroup(0); // 返回空
        } else {
            records = record_dao.findByGroup(group_id_num);
        }
    } else {
        // 私聊：使用数字 ID 查询
        uint64_t user1 = getUserIdNum(*db_, user_id);
        uint64_t user2 = getUserIdNum(*db_, target_id);
        if (user1 == 0 || user2 == 0) {
            std::cerr << "ChatHistory: user not found, user_id=" << user_id 
                      << ", target_id=" << target_id << std::endl;
        } else {
            records = record_dao.findByUserPair(user1, user2);
        }
    }
    
    // 构建JSON响应
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < records.size(); i++) {
        if (i > 0) oss << ",";
        
        // 获取发送者的字符串 user_id
        std::string sender_user_id = getUserIdStr(*db_, records[i].sender_id);
        // 获取接收者的字符串 user_id
        std::string receiver_user_id = records[i].receiver_id > 0 ? 
            getUserIdStr(*db_, records[i].receiver_id) : "";
        
        oss << "{\"id\":" << records[i].id
            << ",\"sender_id\":\"" << sender_user_id << "\""
            << ",\"receiver_id\":\"" << receiver_user_id << "\""
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
    std::cout << "[ChatService] handleAiRequest called: fd=" << conn->fd()
              << ", from_user_id=" << user_id << ", to_user_id=" << target_id
              << ", question(len)=" << question.size() << std::endl;
    
    // 确保用户连接已保存
    {
        std::lock_guard<std::mutex> lock(conn_mutex_);
        user_connections_[user_id] = conn;
        fd_to_user_[conn->fd()] = user_id;
        std::cout << "[ChatService] Mapped fd=" << conn->fd() << " to user_id=" << user_id << std::endl;
    }
    
    // 如果没有指定目标，发送给自己
    if (target_id.empty()) {
        target_id = user_id;
    }
    
    // 保存用户提问到数据库
    uint64_t user_id_num = getUserIdNum(*db_, user_id);
    if (user_id_num > 0) {
        ChatRecord record;
        record.sender_id = user_id_num;
        record.receiver_id = 0;  // AI消息，receiver_id为0
        record.group_id = 0;
        record.content = question;
        record.is_ai = 0;  // 用户提问不是AI回复
        
        ChatRecordDAO record_dao(*db_);
        record_dao.insert(record);
        std::cout << "[ChatService] Saved AI question from user " << user_id << std::endl;
    }
    
    // 创建AI请求并设置extra
    AIRequest ai_request;
    ai_request.user_id = user_id;
    ai_request.target_id = target_id;
    ai_request.question = question;
    ai_request.is_group = is_group;
    ai_request.extra = msg.extra;  // 传递extra，包含chatKey
    
    ai_service_->processRequest(user_id, target_id, question, is_group, msg.extra);
    
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
    
    // 创建AI请求并设置extra
    AIRequest ai_request;
    ai_request.user_id = user_id;
    ai_request.target_id = target_id;
    ai_request.question = question;
    ai_request.is_group = is_group;
    ai_request.extra = msg.extra;  // 传递extra，包含chatKey
    
    ai_service_->processRequest(user_id, target_id, question, is_group, msg.extra);
    
    sendResponse(conn, static_cast<int>(MessageType::AI_AT), 0, 
                "AI request received");
}

/**
 * @brief 处理AI设置修改请求（聊天级别）
 */
void ChatService::handleAiSetting(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string chat_key = getJsonValue(msg.extra, "chatKey");
    std::string nickname = getJsonValue(msg.extra, "nickname");
    std::string tone_str = getJsonValue(msg.extra, "tone");
    std::string priority_str = getJsonValue(msg.extra, "priority");
    
    if (chat_key.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::AI_SETTING), -1, 
                    "Missing chat_key");
        return;
    }
    
    int tone = tone_str.empty() ? 0 : std::stoi(tone_str);
    int priority = priority_str.empty() ? 1 : std::stoi(priority_str);
    
    if (ai_service_->updateChatAISettings(chat_key, nickname, tone, priority, user_id)) {
        // 广播给聊天中的所有成员
        std::string notification = buildResponse(
            static_cast<int>(MessageType::AI_SETTING), 0, 
            "AI settings updated",
            "{\"chat_key\":\"" + chat_key + "\",\"nickname\":\"" + escapeJson(nickname) + 
            "\",\"tone\":" + tone_str + ",\"priority\":" + priority_str + "}");
        
        // 根据chat_key类型广播
        if (chat_key.find("group:") == 0) {
            std::string group_id = chat_key.substr(6);
            broadcastToGroup(group_id, notification);
        } else if (chat_key.find("single:") == 0) {
            std::string target_user_id = chat_key.substr(7);
            broadcastToUser(user_id, notification);
            broadcastToUser(target_user_id, notification);
        } else {
            broadcastToUser(user_id, notification);
        }
        
        sendResponse(conn, static_cast<int>(MessageType::AI_SETTING), 0, 
                    "AI settings updated");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::AI_SETTING), -1, 
                    "Failed to update AI settings");
    }
}

/**
 * @brief 处理修改用户名请求
 */
void ChatService::handleUpdateUsername(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string new_username = getJsonValue(msg.extra, "username");
    
    if (new_username.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::UPDATE_USERNAME), -1, 
                    "Missing username");
        return;
    }
    
    UserDAO user_dao(*db_);
    User* user = user_dao.findByUserId(user_id);
    if (!user) {
        sendResponse(conn, static_cast<int>(MessageType::UPDATE_USERNAME), -1, 
                    "User not found");
        return;
    }
    
    user->username = new_username;
    if (user_dao.update(*user)) {
        sendResponse(conn, static_cast<int>(MessageType::UPDATE_USERNAME), 0, 
                    "Username updated",
                    "{\"username\":\"" + escapeJson(new_username) + "\"}");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::UPDATE_USERNAME), -1, 
                    "Failed to update username");
    }
    
    delete user;
}

/**
 * @brief 处理消息撤回请求
 */
void ChatService::handleMessageRecall(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string message_id_str = getJsonValue(msg.extra, "message_id");
    
    if (message_id_str.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_RECALL), -1, 
                    "Missing message_id");
        return;
    }
    
    uint64_t message_id = strtoull(message_id_str.c_str(), nullptr, 10);
    ChatRecordDAO record_dao(*db_);
    UserDAO user_dao(*db_);
    
    // 检查消息是否属于当前用户
    ChatRecord* record = record_dao.findById(message_id);
    if (!record) {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_RECALL), -1, 
                    "Message not found");
        return;
    }
    
    uint64_t user_id_num = getUserIdNum(*db_, user_id);
    if (record->sender_id != user_id_num) {
        delete record;
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_RECALL), -1, 
                    "Cannot recall others message");
        return;
    }
    
    // 保存receiver_id用于通知
    uint64_t receiver_id = record->receiver_id;
    delete record;
    
    if (record_dao.recall(message_id)) {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_RECALL), 0, 
                    "Message recalled");
        
        // 通知对方消息已撤回
        std::string notification = buildResponse(
            static_cast<int>(MessageType::MESSAGE_RECALL), 0, 
            "Message recalled",
            "{\"message_id\":" + message_id_str + "}");
        
        // 如果是私聊，通知对方
        if (receiver_id > 0) {
            User* receiver = user_dao.findById(receiver_id);
            if (receiver) {
                broadcastToUser(receiver->user_id, notification);
                delete receiver;
            }
        }
    } else {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_RECALL), -1, 
                    "Failed to recall message");
    }
}

/**
 * @brief 处理消息已读请求
 */
void ChatService::handleMessageRead(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string message_id_str = getJsonValue(msg.extra, "message_id");
    
    if (message_id_str.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_READ), -1, 
                    "Missing message_id");
        return;
    }
    
    uint64_t message_id = strtoull(message_id_str.c_str(), nullptr, 10);
    ChatRecordDAO record_dao(*db_);
    
    if (record_dao.markAsRead(message_id)) {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_READ), 0, 
                    "Message marked as read");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::MESSAGE_READ), -1, 
                    "Failed to mark message as read");
    }
}

/**
 * @brief 处理获取好友请求列表请求
 */
void ChatService::handleFriendRequestList(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    
    std::string requests_json = friend_service_->getPendingRequests(user_id);
    
    sendResponse(conn, static_cast<int>(MessageType::FRIEND_REQUEST_LIST), 0, 
                "Success", requests_json);
}

/**
 * @brief 处理设置好友备注请求
 */
void ChatService::handleFriendSetRemark(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string friend_id = msg.to_user_id;
    std::string remark = getJsonValue(msg.extra, "remark");
    
    if (friend_id.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_SET_REMARK), -1, 
                    "Missing friend_id");
        return;
    }
    
    if (friend_service_->setRemark(user_id, friend_id, remark)) {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_SET_REMARK), 0, 
                    "Remark set successfully");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::FRIEND_SET_REMARK), -1, 
                    "Failed to set remark");
    }
}

/**
 * @brief 处理获取加群请求列表请求
 */
void ChatService::handleGroupRequestList(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    
    // 检查是否是群主
    if (!group_service_->isGroupCreator(user_id, group_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_REQUEST_LIST), -1, 
                    "Not group creator");
        return;
    }
    
    std::string requests_json = group_service_->getPendingRequests(group_id);
    
    sendResponse(conn, static_cast<int>(MessageType::GROUP_REQUEST_LIST), 0, 
                "Success", requests_json);
}

/**
 * @brief 处理修改群名请求
 */
void ChatService::handleGroupModifyName(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    std::string new_name = getJsonValue(msg.extra, "group_name");
    
    if (new_name.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MODIFY_NAME), -1, 
                    "Missing group_name");
        return;
    }
    
    // 检查是否是群主或管理员
    if (!group_service_->isGroupCreator(user_id, group_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MODIFY_NAME), -1, 
                    "Not group creator");
        return;
    }
    
    if (group_service_->modifyGroupName(group_id, new_name, user_id)) {
        // 广播群名修改通知
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_MODIFY_NAME), 0, 
            "Group name changed",
            "{\"group_id\":\"" + group_id + "\",\"group_name\":\"" + escapeJson(new_name) + "\"}");
        broadcastToGroup(group_id, notification);
        
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MODIFY_NAME), 0, 
                    "Group name modified");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_MODIFY_NAME), -1, 
                    "Failed to modify group name");
    }
}

/**
 * @brief 处理退出群聊请求
 */
void ChatService::handleGroupLeave(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    
    // 群主不能退出群聊
    if (group_service_->isGroupCreator(user_id, group_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_LEAVE), -1, 
                    "Creator cannot leave group");
        return;
    }
    
    if (group_service_->leaveGroup(user_id, group_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_LEAVE), 0, 
                    "Left group successfully");
        
        // 通知群成员
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_LEAVE), 0, 
            "Member left group",
            "{\"user_id\":\"" + user_id + "\",\"group_id\":\"" + group_id + "\"}");
        broadcastToGroup(group_id, notification);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_LEAVE), -1, 
                    "Failed to leave group");
    }
}

/**
 * @brief 处理解散群聊请求
 */
void ChatService::handleGroupDissolve(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    
    // 先广播通知给所有群成员（在删除群聊之前）
    std::string notification = buildResponse(
        static_cast<int>(MessageType::GROUP_DISSOLVE), 0, 
        "Group dissolved",
        "{\"group_id\":\"" + group_id + "\"}");
    broadcastToGroup(group_id, notification);
    
    // 解散群聊
    if (group_service_->dissolveGroup(group_id, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_DISSOLVE), 0, 
                    "Group dissolved successfully");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_DISSOLVE), -1, 
                    "Failed to dissolve group");
    }
}

/**
 * @brief 处理踢出群成员请求
 */
void ChatService::handleGroupKick(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;  // 操作者
    std::string target_id = msg.to_user_id;  // 被踢者
    std::string group_id = getJsonValue(msg.extra, "group_id");
    
    // 检查是否是群主
    if (!group_service_->isGroupCreator(user_id, group_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_KICK), -1, 
                    "Not group creator");
        return;
    }
    
    // 不能踢自己
    if (user_id == target_id) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_KICK), -1, 
                    "Cannot kick yourself");
        return;
    }
    
    if (group_service_->kickMember(target_id, group_id, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_KICK), 0, 
                    "Member kicked");
        
        // 通知群成员
        std::string notification = buildResponse(
            static_cast<int>(MessageType::GROUP_KICK), 0, 
            "Member kicked from group",
            "{\"user_id\":\"" + target_id + "\",\"group_id\":\"" + group_id + "\"}");
        broadcastToGroup(group_id, notification);
        
        // 通知被踢者
        std::string kick_notification = buildResponse(
            static_cast<int>(MessageType::GROUP_KICK), 0, 
            "You have been kicked from group",
            "{\"group_id\":\"" + group_id + "\"}");
        broadcastToUser(target_id, kick_notification);
    } else {
        sendResponse(conn, static_cast<int>(MessageType::GROUP_KICK), -1, 
                    "Failed to kick member");
    }
}

/**
 * @brief 处理上传用户头像请求
 */
void ChatService::handleUploadAvatar(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string avatar_data = getJsonValue(msg.extra, "avatar_data");  // Base64编码的图片数据
    std::string format = getJsonValue(msg.extra, "format");  // jpg/png
    
    if (avatar_data.empty()) {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_AVATAR), -1, 
                    "Missing avatar data");
        return;
    }
    
    // 生成文件名和路径
    std::string filename = "avatar_" + user_id + "." + (format.empty() ? "png" : format);
    std::string filepath = "./backend/static/avatars/" + filename;
    
    // 确保目录存在
    mkdir("./backend/static/avatars", 0755);
    
    // 提取 Base64 数据（去掉 "data:image/xxx;base64," 前缀）
    std::string base64_content = avatar_data;
    size_t comma_pos = avatar_data.find(',');
    if (comma_pos != std::string::npos) {
        base64_content = avatar_data.substr(comma_pos + 1);
    }
    
    // 解码 Base64 并写入文件
    // Base64 解码：每4个字符转3个字节
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<unsigned char> decoded;
    int val = 0, bits = -8;
    for (char c : base64_content) {
        if (c == '=') break;
        size_t pos = chars.find(c);
        if (pos == std::string::npos) continue;
        val = (val << 6) + pos;
        bits += 6;
        if (bits >= 0) {
            decoded.push_back((val >> bits) & 0xFF);
            bits -= 8;
        }
    }
    
    // 写入文件
    FILE* fp = fopen(filepath.c_str(), "wb");
    if (fp) {
        fwrite(decoded.data(), 1, decoded.size(), fp);
        fclose(fp);
        std::cout << "Avatar saved to " << filepath << " (" << decoded.size() << " bytes)" << std::endl;
    } else {
        std::cerr << "Failed to write avatar file: " << filepath << std::endl;
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_AVATAR), -1, 
                    "Failed to save avatar file");
        return;
    }
    
    // 更新数据库
    UserDAO user_dao(*db_);
    User* user = user_dao.findByUserId(user_id);
    if (!user) {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_AVATAR), -1, 
                    "User not found");
        return;
    }
    
    user->avatar_path = "/static/avatars/" + filename;
    if (user_dao.update(*user)) {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_AVATAR), 0, 
                    "Avatar uploaded",
                    "{\"avatar_path\":\"" + user->avatar_path + "\"}");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_AVATAR), -1, 
                    "Failed to upload avatar");
    }
    
    delete user;
}

/**
 * @brief 处理上传群聊头像请求（简化实现）
 */
void ChatService::handleUploadGroupAvatar(spConnection conn, const Message& msg) {
    std::string user_id = msg.from_user_id;
    std::string group_id = msg.to_user_id;
    std::string avatar_data = getJsonValue(msg.extra, "avatar_data");
    std::string format = getJsonValue(msg.extra, "format");
    
    // 检查是否是群主
    if (!group_service_->isGroupCreator(user_id, group_id)) {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_GROUP_AVATAR), -1, 
                    "Not group creator");
        return;
    }
    
    // 生成文件名
    std::string filename = "group_" + group_id + "." + (format.empty() ? "png" : format);
    std::string filepath = "./backend/static/avatars/" + filename;
    
    // 简化实现：直接更新数据库路径
    if (group_service_->modifyGroupAvatar(group_id, "/static/avatars/" + filename, user_id)) {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_GROUP_AVATAR), 0, 
                    "Group avatar uploaded",
                    "{\"avatar_path\":\"/static/avatars/" + filename + "\"}");
    } else {
        sendResponse(conn, static_cast<int>(MessageType::UPLOAD_GROUP_AVATAR), -1, 
                    "Failed to upload group avatar");
    }
}

/**
 * @brief 广播消息给指定用户
 */
void ChatService::broadcastToUser(const std::string& user_id, const std::string& message) {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    
    auto it = user_connections_.find(user_id);
    if (it != user_connections_.end()) {
        try {
            it->second->send(message.data(), message.size());
        } catch (const std::exception& e) {
            std::cerr << "[ChatService] Failed to send message to " << user_id << ": " << e.what() << std::endl;
            user_connections_.erase(it);
        }
    }
}

/**
 * @brief 广播消息给群聊所有成员
 */
void ChatService::broadcastToGroup(const std::string& group_id, const std::string& message, 
                                   const std::string& exclude_user_id) {
    // 先在锁外获取所有成员的user_id字符串
    std::vector<std::string> member_user_ids = group_service_->getGroupMemberUserIds(group_id);
    
    // 在锁内只做发送
    std::lock_guard<std::mutex> lock(conn_mutex_);
    
    for (const auto& member_user_id : member_user_ids) {
        if (member_user_id == exclude_user_id) continue;
        
        auto it = user_connections_.find(member_user_id);
        if (it != user_connections_.end()) {
            try {
                it->second->send(message.data(), message.size());
            } catch (const std::exception& e) {
                std::cerr << "[ChatService] Failed to send to " << member_user_id << ": " << e.what() << std::endl;
            }
        }
    }
}

/**
 * @brief 检查用户是否在线
 */
bool ChatService::isUserOnline(const std::string& user_id) {
    std::lock_guard<std::mutex> lock(conn_mutex_);
    return user_connections_.find(user_id) != user_connections_.end();
}
