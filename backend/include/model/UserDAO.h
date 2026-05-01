/**
 * @file UserDAO.h
 * @brief 用户数据访问对象
 * 
 * 提供用户表的CRUD操作及密码验证功能。
 */

#pragma once

#include "model/User.h"
#include "module/Database.h"
#include <vector>

/**
 * @class UserDAO
 * @brief 用户数据访问对象
 */
class UserDAO {
public:
    /**
     * @brief 构造函数
     * @param db 数据库连接引用
     */
    UserDAO(Database& db);
    
    /**
     * @brief 插入用户记录
     * @param user 用户对象（插入后会自动填充id）
     * @return 插入成功返回true，失败返回false
     */
    bool insert(User& user);
    
    /**
     * @brief 更新用户记录
     * @param user 用户对象
     * @return 更新成功返回true，失败返回false
     */
    bool update(const User& user);
    
    /**
     * @brief 删除用户记录
     * @param id 用户主键ID
     * @return 删除成功返回true，失败返回false
     */
    bool remove(uint64_t id);
    
    /**
     * @brief 根据主键ID查询用户
     * @param id 用户主键ID
     * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
     */
    User* findById(uint64_t id);
    
    /**
     * @brief 根据用户ID查询用户
     * @param user_id 用户ID
     * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
     */
    User* findByUserId(const std::string& user_id);
    
    /**
     * @brief 根据手机号查询用户
     * @param phone 手机号
     * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
     */
    User* findByPhone(const std::string& phone);
    
    /**
     * @brief 查询所有用户
     * @return 用户列表
     */
    std::vector<User> findAll();
    
    /**
     * @brief 验证密码
     * @param user_id 用户ID
     * @param password 明文密码
     * @return 密码正确返回true，否则返回false
     */
    bool verifyPassword(const std::string& user_id, const std::string& password);
    
private:
    Database& db_; ///< 数据库连接引用
};
