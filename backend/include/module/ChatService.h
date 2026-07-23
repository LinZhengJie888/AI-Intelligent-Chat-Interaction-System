/**
 * @file ChatService.h
 * @brief 消息路由中心模块
 * 
 * 作为消息路由中心，对接Reactor的消息回调，区分不同类型消息并分发到对应业务模块。
 * 支持登录、注册、聊天、加好友、AI请求等消息类型的处理。
 */

#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include "reactor/Connection.h"

// 前向声明
class VerifyService;
class FriendService;
class GroupService;
class AiService;
class Database;

/**
 * @enum MessageType
 * @brief 消息类型枚举
 */
enum class MessageType {
    // 用户相关
    LOGIN = 1,              ///< 登录请求
    REGISTER = 2,           ///< 注册请求
    LOGOUT = 3,             ///< 登出请求
    
    // 验证码相关
    GET_CAPTCHA = 10,       ///< 获取验证码
    
    // 好友相关
    FRIEND_ADD = 20,        ///< 添加好友请求
    FRIEND_AGREE = 21,      ///< 同意好友请求
    FRIEND_REJECT = 22,     ///< 拒绝好友请求
    FRIEND_LIST = 23,       ///< 获取好友列表
    FRIEND_DELETE = 24,     ///< 删除好友
    FRIEND_REQUEST_LIST = 25, ///< 获取好友请求列表
    FRIEND_SET_REMARK = 26, ///< 设置好友备注
    
    // 群聊相关
    GROUP_CREATE = 30,      ///< 创建群聊
    GROUP_JOIN = 31,        ///< 加入群聊申请
    GROUP_AGREE = 32,       ///< 同意加群申请
    GROUP_REJECT = 33,      ///< 拒绝加群申请
    GROUP_MESSAGE = 34,     ///< 群聊消息
    GROUP_MEMBERS = 35,     ///< 获取群成员列表
    GROUP_LIST = 36,        ///< 获取群列表
    GROUP_MODIFY_NAME = 37, ///< 修改群名
    GROUP_LEAVE = 39,       ///< 退出群聊
    
    // 聊天相关
    CHAT_PRIVATE = 40,      ///< 私聊消息
    CHAT_HISTORY = 41,      ///< 获取聊天记录
    MESSAGE_RECALL = 42,    ///< 消息撤回
    MESSAGE_READ = 43,      ///< 消息已读
    
    // AI相关
    AI_REQUEST = 50,        ///< AI请求（按键调用）
    AI_AT = 51,             ///< AI请求（@召唤）
    AI_SETTING = 52,        ///< AI设置修改
    AI_STREAM_START = 53,   ///< AI流式输出开始
    AI_STREAM_CHUNK = 54,   ///< AI流式输出增量
    AI_STREAM_END = 55,     ///< AI流式输出结束
    
    // 用户信息修改
    UPDATE_USERNAME = 62,   ///< 修改用户名
    GROUP_KICK = 63,        ///< 踢出群成员
    GROUP_REQUEST_LIST = 64, ///< 获取加群请求列表
    GROUP_DISSOLVE = 65,    ///< 解散群聊
    
    // 头像相关
    UPLOAD_AVATAR = 70,     ///< 上传用户头像
    UPLOAD_GROUP_AVATAR = 71, ///< 上传群聊头像
    
    // 响应
    RESPONSE_OK = 100,      ///< 成功响应
    RESPONSE_ERROR = 101    ///< 错误响应
};

/**
 * @struct Message
 * @brief 消息数据结构
 */
struct Message {
    int type;                       ///< 消息类型
    std::string from_user_id;       ///< 发送方用户ID
    std::string to_user_id;         ///< 接收方用户ID（用户ID或群聊ID）
    std::string content;            ///< 消息内容
    std::string extra;              ///< 额外数据（JSON格式）
    std::string timestamp;          ///< 时间戳
    
    Message() : type(0) {}
};

/**
 * @class ChatService
 * @brief 消息路由中心类
 * 
 * 负责解析客户端消息，根据消息类型分发到对应的业务模块处理。
 */
class ChatService {
public:
    /**
     * @brief 获取单例实例
     * @return ChatService单例引用
     */
    static ChatService& getInstance();
    
    /**
     * @brief 初始化服务
     * @param db 数据库连接引用
     * @return 初始化成功返回true，失败返回false
     */
    bool init(Database& db);
    
    /**
     * @brief 处理新连接
     * @param conn 连接对象
     */
    void handleNewConnection(spConnection conn);
    
    /**
     * @brief 处理连接关闭
     * @param conn 连接对象
     */
    void handleClose(spConnection conn);
    
    /**
     * @brief 处理消息
     * @param conn 连接对象
     * @param message 消息内容
     */
    void handleMessage(spConnection conn, std::string& message);
    
    /**
     * @brief 处理发送完成
     * @param conn 连接对象
     */
    void handleSendComplete(spConnection conn);
    
    /**
     * @brief 处理超时
     * @param conn 连接对象
     */
    void handleTimeout(spConnection conn);
    
    /**
     * @brief 广播消息给指定用户
     * @param user_id 用户ID
     * @param message 消息内容
     */
    void broadcastToUser(const std::string& user_id, const std::string& message);
    
    /**
     * @brief 广播消息给群聊所有成员
     * @param group_id 群聊ID
     * @param message 消息内容
     * @param exclude_user_id 排除的用户ID（可选）
     */
    void broadcastToGroup(const std::string& group_id, const std::string& message, 
                         const std::string& exclude_user_id = "");

    /**
     * @brief 检查用户是否在线
     * @param user_id 用户ID
     * @return 在线返回true，否则false
     */
    bool isUserOnline(const std::string& user_id);

private:
    /**
     * @brief 私有构造函数
     */
    ChatService();
    
    /**
     * @brief 析构函数
     */
    ~ChatService();
    
    /**
     * @brief 禁用拷贝构造函数
     */
    ChatService(const ChatService&) = delete;
    
    /**
     * @brief 禁用赋值运算符
     */
    ChatService& operator=(const ChatService&) = delete;
    
    /**
     * @brief 解析消息JSON
     * @param json_str JSON字符串
     * @param msg 消息结构体输出
     * @return 解析成功返回true，失败返回false
     */
    bool parseMessage(const std::string& json_str, Message& msg);
    
    /**
     * @brief 构建响应JSON
     * @param type 响应类型
     * @param code 状态码
     * @param msg 提示消息
     * @param data 数据内容
     * @return JSON字符串
     */
    std::string buildResponse(int type, int code, const std::string& msg, 
                             const std::string& data = "");
    
    /**
     * @brief 发送响应给客户端
     * @param conn 连接对象
     * @param type 响应类型
     * @param code 状态码
     * @param msg 提示消息
     * @param data 数据内容
     */
    void sendResponse(spConnection conn, int type, int code, 
                     const std::string& msg, const std::string& data = "");
    
    /**
     * @brief 处理登录请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleLogin(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理注册请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleRegister(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理登出请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleLogout(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取验证码请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGetCaptcha(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理添加好友请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendAdd(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理同意好友请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendAgree(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理拒绝好友请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendReject(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取好友列表请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendList(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理删除好友请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendDelete(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理创建群聊请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupCreate(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理加群申请请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupJoin(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理同意加群申请请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupAgree(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理拒绝加群申请请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupReject(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理群聊消息请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupMessage(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取群成员列表请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupMembers(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取群列表请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupList(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理私聊消息请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handlePrivateChat(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取聊天记录请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleChatHistory(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理AI请求（按键调用）
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleAiRequest(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理AI请求（@召唤）
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleAiAt(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理AI设置修改请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleAiSetting(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理修改用户名请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleUpdateUsername(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理消息撤回请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleMessageRecall(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理消息已读请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleMessageRead(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取好友请求列表请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendRequestList(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理设置好友备注请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleFriendSetRemark(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理获取加群请求列表请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupRequestList(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理修改群名请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupModifyName(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理退出群聊请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupLeave(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理踢出群成员请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupKick(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理解散群聊请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleGroupDissolve(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理上传用户头像请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleUploadAvatar(spConnection conn, const Message& msg);
    
    /**
     * @brief 处理上传群聊头像请求
     * @param conn 连接对象
     * @param msg 消息结构体
     */
    void handleUploadGroupAvatar(spConnection conn, const Message& msg);

    // 业务模块指针
    std::unique_ptr<VerifyService> verify_service_;
    std::unique_ptr<FriendService> friend_service_;
    std::unique_ptr<GroupService> group_service_;
    std::unique_ptr<AiService> ai_service_;
    
    // 用户ID到连接的映射（用于消息推送）
    std::map<std::string, spConnection> user_connections_;
    std::mutex conn_mutex_;
    
    // 连接ID到用户ID的映射
    std::map<int, std::string> fd_to_user_;
    
    Database* db_;
    bool initialized_;
};
