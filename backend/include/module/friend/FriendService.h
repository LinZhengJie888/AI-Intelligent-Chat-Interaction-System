/**
 * @file FriendService.h
 * @brief 好友管理服务模块
 * 
 * 实现好友添加、同意、拒绝、删除、好友列表查询等功能。
 * 支持微信式的好友添加逻辑，包含冷却期机制。
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>

class Database;
struct User;
struct FriendRequest;
struct FriendRelation;

/**
 * @struct FriendInfo
 * @brief 好友信息结构体
 */
struct FriendInfo {
    std::string user_id;        ///< 用户ID
    std::string username;       ///< 用户名
    std::string nickname;       ///< 昵称
    std::string avatar_path;    ///< 头像路径
    std::string remark;         ///< 备注名
    std::string add_time;       ///< 添加时间
};

/**
 * @struct FriendRequestInfo
 * @brief 好友请求信息结构体
 */
struct FriendRequestInfo {
    uint64_t request_id;        ///< 请求ID
    std::string from_user_id;   ///< 请求方用户ID
    std::string from_username;  ///< 请求方用户名
    std::string from_avatar;    ///< 请求方头像
    std::string to_user_id;     ///< 接收方用户ID
    std::string request_msg;    ///< 请求消息
    int8_t status;              ///< 状态（0-待处理，1-同意，2-拒绝）
    std::string create_time;    ///< 创建时间
    std::string cooling_time;   ///< 冷却时间
};

/**
 * @class FriendService
 * @brief 好友管理服务类
 * 
 * 负责处理好友相关的业务逻辑，包括添加、同意、拒绝、删除好友等。
 */
class FriendService {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    FriendService(Database& db);
    
    /**
     * @brief 析构函数
     */
    ~FriendService();
    
    /**
     * @brief 初始化服务
     * @return 初始化成功返回true，失败返回false
     */
    bool init();
    
    /**
     * @brief 发送好友请求
     * @param from_user_id 请求方用户ID
     * @param to_user_id 接收方用户ID
     * @param request_msg 请求消息
     * @return 0-成功，-1-失败，-2-已是好友，-3-请求已存在，-4-冷却期
     */
    int sendRequest(const std::string& from_user_id, const std::string& to_user_id, 
                   const std::string& request_msg);
    
    /**
     * @brief 同意好友请求
     * @param from_user_id 请求方用户ID
     * @param to_user_id 接收方用户ID（同意者）
     * @return 操作成功返回true，失败返回false
     */
    bool agreeRequest(const std::string& from_user_id, const std::string& to_user_id);
    
    /**
     * @brief 拒绝好友请求
     * @param from_user_id 请求方用户ID
     * @param to_user_id 接收方用户ID（拒绝者）
     * @return 操作成功返回true，失败返回false
     */
    bool rejectRequest(const std::string& from_user_id, const std::string& to_user_id);
    
    /**
     * @brief 删除好友
     * @param user_id 用户ID
     * @param friend_id 好友用户ID
     * @return 操作成功返回true，失败返回false
     */
    bool deleteFriend(const std::string& user_id, const std::string& friend_id);
    
    /**
     * @brief 检查是否是好友关系
     * @param user_id1 用户1 ID
     * @param user_id2 用户2 ID
     * @return 是好友返回true，否则返回false
     */
    bool isFriend(const std::string& user_id1, const std::string& user_id2);
    
    /**
     * @brief 获取好友列表
     * @param user_id 用户ID
     * @return 好友列表JSON字符串
     */
    std::string getFriendList(const std::string& user_id);
    
    /**
     * @brief 获取好友列表（结构体形式）
     * @param user_id 用户ID
     * @return 好友信息列表
     */
    std::vector<FriendInfo> getFriendListStruct(const std::string& user_id);
    
    /**
     * @brief 获取待处理的好友请求列表
     * @param user_id 用户ID
     * @return 好友请求列表JSON字符串
     */
    std::string getPendingRequests(const std::string& user_id);
    
    /**
     * @brief 获取待处理的好友请求列表（结构体形式）
     * @param user_id 用户ID
     * @return 好友请求信息列表
     */
    std::vector<FriendRequestInfo> getPendingRequestsStruct(const std::string& user_id);
    
    /**
     * @brief 设置好友备注
     * @param user_id 用户ID
     * @param friend_id 好友用户ID
     * @param remark 备注名
     * @return 操作成功返回true，失败返回false
     */
    bool setRemark(const std::string& user_id, const std::string& friend_id, 
                  const std::string& remark);
    
    /**
     * @brief 获取好友数量
     * @param user_id 用户ID
     * @return 好友数量
     */
    int getFriendCount(const std::string& user_id);

private:
    /**
     * @brief 创建好友关系表
     * @return 创建成功返回true，失败返回false
     */
    bool createFriendRelationTable();
    
    /**
     * @brief 创建好友请求表
     * @return 创建成功返回true，失败返回false
     */
    bool createFriendRequestTable();
    
    /**
     * @brief 检查冷却期
     * @param from_user_id 请求方用户ID
     * @param to_user_id 接收方用户ID
     * @return 在冷却期内返回true，否则返回false
     */
    bool isInCoolingPeriod(const std::string& from_user_id, const std::string& to_user_id);
    
    /**
     * @brief 添加好友关系（双向）
     * @param user_id1 用户1 ID
     * @param user_id2 用户2 ID
     * @return 添加成功返回true，失败返回false
     */
    bool addFriendRelation(const std::string& user_id1, const std::string& user_id2);
    
    /**
     * @brief 删除好友关系（双向）
     * @param user_id1 用户1 ID
     * @param user_id2 用户2 ID
     * @return 删除成功返回true，失败返回false
     */
    bool removeFriendRelation(const std::string& user_id1, const std::string& user_id2);
    
    /**
     * @brief 更新好友请求状态
     * @param from_user_id 请求方用户ID
     * @param to_user_id 接收方用户ID
     * @param status 新状态
     * @return 更新成功返回true，失败返回false
     */
    bool updateRequestStatus(const std::string& from_user_id, const std::string& to_user_id, 
                            int8_t status);
    
    /**
     * @brief 设置冷却期
     * @param from_user_id 请求方用户ID
     * @param to_user_id 接收方用户ID
     * @return 设置成功返回true，失败返回false
     */
    bool setCoolingPeriod(const std::string& from_user_id, const std::string& to_user_id);

    Database& db_;  ///< 数据库连接引用
    
    static const int COOLING_PERIOD_HOURS = 1;  ///< 冷却期时间（小时）
};
