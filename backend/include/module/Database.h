/**
 * @file Database.h
 * @brief MySQL数据库连接封装类
 * 
 * 封装MySQL C API，提供数据库连接、查询、执行等操作的简洁接口。
 */

#pragma once

#include <string>
#include <mysql/mysql.h>

/**
 * @class Database
 * @brief MySQL数据库连接封装类
 */
class Database {
public:
    /**
     * @brief 构造函数，初始化MySQL连接对象
     */
    Database();
    
    /**
     * @brief 析构函数，断开数据库连接
     */
    ~Database();
    
    /**
     * @brief 连接到MySQL数据库
     * @param host 数据库主机地址
     * @param port 数据库端口
     * @param user 数据库用户名
     * @param password 数据库密码
     * @param dbname 数据库名称
     * @return 连接成功返回true，失败返回false
     */
    bool connect(const std::string& host, int port, 
                 const std::string& user, const std::string& password, 
                 const std::string& dbname);
    
    /**
     * @brief 断开数据库连接
     */
    void disconnect();
    
    /**
     * @brief 获取MySQL连接指针
     * @return MySQL连接指针
     */
    MYSQL* getConnection();
    
    /**
     * @brief 检查是否已连接
     * @return 已连接返回true，否则返回false
     */
    bool isConnected() const;
    
    /**
     * @brief 执行SQL语句（INSERT、UPDATE、DELETE等）
     * @param sql SQL语句
     * @return 执行成功返回true，失败返回false
     */
    bool execute(const std::string& sql);
    
    /**
     * @brief 执行查询SQL语句（SELECT）
     * @param sql SQL查询语句
     * @return 查询结果集，失败返回nullptr
     */
    MYSQL_RES* query(const std::string& sql);
    
    /**
     * @brief 释放查询结果集
     * @param res 查询结果集
     */
    void freeResult(MYSQL_RES* res);

private:
    MYSQL* connection_;   ///< MySQL连接指针
    bool connected_;      ///< 连接状态
};
