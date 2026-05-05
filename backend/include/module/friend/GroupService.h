/**
 * @file GroupService.h
 * @brief 群聊管理服务模块
 * 
 * 实现群聊创建、加群申请、群消息推送、群成员管理等功能。
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>

class Database;
struct GroupChat;
struct GroupMember;
struct GroupRequest;

/**
 * @struct GroupInfo
 * @brief 群聊信息结构体
 */
struct GroupInfo {
    std::string group_id;       ///< 群聊ID
    std::string group_name;     ///< 群名
    std::string creator_id;     ///< 创建者用户ID
    std::string avatar_path;    ///< 群头像路径
    std::string announcement;   ///< 群公告
    std::string create_time;    ///< 创建时间
    int member_count;           ///< 成员数量
};

/**
 * @struct GroupMemberInfo
 * @brief 群成员信息结构体
 */
struct GroupMemberInfo {
    std::string user_id;        ///< 用户ID
    std::string username;       ///< 用户名
    std::string nickname;       ///< 昵称
    std::string avatar_path;    ///< 头像路径
    int8_t role;                ///< 角色（0-普通成员，1-管理员，2-群主）
    std::string join_time;      ///< 加入时间
};

/**
 * @struct GroupRequestInfo
 * @brief 加群请求信息结构体
 */
struct GroupRequestInfo {
    uint64_t request_id;        ///< 请求ID
    std::string group_id;       ///< 群聊ID
    std::string group_name;     ///< 群名
    std::string from_user_id;   ///< 申请人用户ID
    std::string from_username;  ///< 申请人用户名
    std::string from_avatar;    ///< 申请人头像
    std::string request_msg;    ///< 请求消息
    int8_t status;              ///< 状态（0-待处理，1-同意，2-拒绝）
    std::string create_time;    ///< 创建时间
};

/**
 * @class GroupService
 * @brief 群聊管理服务类
 * 
 * 负责处理群聊相关的业务逻辑，包括创建群聊、加群申请、群消息推送等。
 */
class GroupService {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    GroupService(Database& db);
    
    /**
     * @brief 析构函数
     */
    ~GroupService();
    
    /**
     * @brief 初始化服务
     * @return 初始化成功返回true，失败返回false
     */
    bool init();
    
    /**
     * @brief 创建群聊
     * @param creator_id 创建者用户ID
     * @param group_name 群名
     * @param group_id 输出的群聊ID
     * @return 创建成功返回true，失败返回false
     */
    bool createGroup(const std::string& creator_id, const std::string& group_name, 
                    std::string& group_id);
    
    /**
     * @brief 发送加群申请
     * @param user_id 申请人用户ID
     * @param group_id 群聊ID
     * @param request_msg 申请消息
     * @return 0-成功，-1-失败，-2-已是成员，-3-申请已存在
     */
    int sendJoinRequest(const std::string& user_id, const std::string& group_id, 
                       const std::string& request_msg);
    
    /**
     * @brief 同意加群申请
     * @param user_id 申请人用户ID
     * @param group_id 群聊ID
     * @param operator_id 操作者用户ID（群主）
     * @return 操作成功返回true，失败返回false
     */
    bool agreeJoinRequest(const std::string& user_id, const std::string& group_id, 
                         const std::string& operator_id);
    
    /**
     * @brief 拒绝加群申请
     * @param user_id 申请人用户ID
     * @param group_id 群聊ID
     * @param operator_id 操作者用户ID（群主）
     * @return 操作成功返回true，失败返回false
     */
    bool rejectJoinRequest(const std::string& user_id, const std::string& group_id, 
                          const std::string& operator_id);
    
    /**
     * @brief 发送群消息
     * @param sender_id 发送者用户ID
     * @param group_id 群聊ID
     * @param content 消息内容
     * @return 发送成功返回true，失败返回false
     */
    bool sendMessage(const std::string& sender_id, const std::string& group_id, 
                    const std::string& content);
    
    /**
     * @brief 获取群聊信息
     * @param group_id 群聊ID
     * @return 群聊信息JSON字符串
     */
    std::string getGroupInfo(const std::string& group_id);
    
    /**
     * @brief 获取群聊信息（结构体形式）
     * @param group_id 群聊ID
     * @return 群聊信息结构体
     */
    GroupInfo getGroupInfoStruct(const std::string& group_id);
    
    /**
     * @brief 获取群成员列表
     * @param group_id 群聊ID
     * @return 群成员列表JSON字符串
     */
    std::string getGroupMembers(const std::string& group_id);
    
    /**
     * @brief 获取群成员列表（结构体形式）
     * @param group_id 群聊ID
     * @return 群成员信息列表
     */
    std::vector<GroupMemberInfo> getGroupMembersStruct(const std::string& group_id);
    
    /**
     * @brief 获取群成员列表（用于ChatService广播）
     * @param group_id 群聊ID
     * @return 群成员列表
     */
    std::vector<GroupMember> getGroupMembersList(const std::string& group_id);
    
    /**
     * @brief 获取用户加入的群聊列表
     * @param user_id 用户ID
     * @return 群聊列表JSON字符串
     */
    std::string getUserGroups(const std::string& user_id);
    
    /**
     * @brief 获取用户加入的群聊列表（结构体形式）
     * @param user_id 用户ID
     * @return 群聊信息列表
     */
    std::vector<GroupInfo> getUserGroupsStruct(const std::string& user_id);
    
    /**
     * @brief 获取待处理的加群申请列表
     * @param group_id 群聊ID
     * @return 加群申请列表JSON字符串
     */
    std::string getPendingRequests(const std::string& group_id);
    
    /**
     * @brief 获取待处理的加群申请列表（结构体形式）
     * @param group_id 群聊ID
     * @return 加群申请信息列表
     */
    std::vector<GroupRequestInfo> getPendingRequestsStruct(const std::string& group_id);
    
    /**
     * @brief 获取群聊创建者
     * @param group_id 群聊ID
     * @return 创建者用户ID
     */
    std::string getGroupCreator(const std::string& group_id);
    
    /**
     * @brief 检查用户是否是群成员
     * @param user_id 用户ID
     * @param group_id 群聊ID
     * @return 是群成员返回true，否则返回false
     */
    bool isGroupMember(const std::string& user_id, const std::string& group_id);
    
    /**
     * @brief 检查用户是否是群主
     * @param user_id 用户ID
     * @param group_id 群聊ID
     * @return 是群主返回true，否则返回false
     */
    bool isGroupCreator(const std::string& user_id, const std::string& group_id);
    
    /**
     * @brief 修改群名
     * @param group_id 群聊ID
     * @param new_name 新群名
     * @param operator_id 操作者用户ID
     * @return 修改成功返回true，失败返回false
     */
    bool modifyGroupName(const std::string& group_id, const std::string& new_name, 
                        const std::string& operator_id);
    
    /**
     * @brief 修改群头像
     * @param group_id 群聊ID
     * @param avatar_path 新头像路径
     * @param operator_id 操作者用户ID
     * @return 修改成功返回true，失败返回false
     */
    bool modifyGroupAvatar(const std::string& group_id, const std::string& avatar_path, 
                          const std::string& operator_id);
    
    /**
     * @brief 退出群聊
     * @param user_id 用户ID
     * @param group_id 群聊ID
     * @return 退出成功返回true，失败返回false
     */
    bool leaveGroup(const std::string& user_id, const std::string& group_id);
    
    /**
     * @brief 踢出群成员
     * @param user_id 被踢用户ID
     * @param group_id 群聊ID
     * @param operator_id 操作者用户ID（群主或管理员）
     * @return 踢出成功返回true，失败返回false
     */
    bool kickMember(const std::string& user_id, const std::string& group_id, 
                   const std::string& operator_id);

private:
    /**
     * @brief 创建群聊表
     * @return 创建成功返回true，失败返回false
     */
    bool createGroupChatTable();
    
    /**
     * @brief 创建群成员表
     * @return 创建成功返回true，失败返回false
     */
    bool createGroupMemberTable();
    
    /**
     * @brief 创建加群请求表
     * @return 创建成功返回true，失败返回false
     */
    bool createGroupRequestTable();
    
    /**
     * @brief 创建群聊记录表
     * @return 创建成功返回true，失败返回false
     */
    bool createGroupRecordTable();
    
    /**
     * @brief 生成唯一群聊ID
     * @return 群聊ID字符串
     */
    std::string generateGroupId();
    
    /**
     * @brief 添加群成员
     * @param group_id 群聊ID
     * @param user_id 用户ID
     * @param role 角色
     * @return 添加成功返回true，失败返回false
     */
    bool addGroupMember(const std::string& group_id, const std::string& user_id, int8_t role);
    
    /**
     * @brief 移除群成员
     * @param group_id 群聊ID
     * @param user_id 用户ID
     * @return 移除成功返回true，失败返回false
     */
    bool removeGroupMember(const std::string& group_id, const std::string& user_id);
    
    /**
     * @brief 更新加群请求状态
     * @param user_id 申请人用户ID
     * @param group_id 群聊ID
     * @param status 新状态
     * @return 更新成功返回true，失败返回false
     */
    bool updateRequestStatus(const std::string& user_id, const std::string& group_id, 
                            int8_t status);

    Database& db_;  ///< 数据库连接引用
};
