/**
 * @file Database.cpp
 * @brief MySQL数据库连接封装实现
 */

#include "module/Database.h"
#include <cstdio>

/**
 * @brief 构造函数，初始化MySQL连接
 */
Database::Database() : connection_(nullptr), connected_(false) {
    connection_ = mysql_init(nullptr);
}

/**
 * @brief 析构函数，断开数据库连接
 */
Database::~Database() {
    disconnect();
}

/**
 * @brief 连接到MySQL数据库
 * @param host 数据库主机地址
 * @param port 数据库端口
 * @param user 数据库用户名
 * @param password 数据库密码
 * @param dbname 数据库名称
 * @return 连接成功返回true，失败返回false
 */
bool Database::connect(const std::string& host, int port,
                       const std::string& user, const std::string& password,
                       const std::string& dbname) {
    if (connected_) {
        disconnect();
    }
    
    if (!connection_) {
        return false;
    }
    
    // 设置自动重连选项
    bool reconnect = true;
    mysql_options(connection_, MYSQL_OPT_RECONNECT, &reconnect);
    
    // 连接数据库
    if (!mysql_real_connect(connection_, host.c_str(), user.c_str(),
                           password.c_str(), dbname.c_str(), port, nullptr, 0)) {
        fprintf(stderr, "MySQL connect error: %s\n", mysql_error(connection_));
        return false;
    }
    
    // 设置字符集为utf8mb4，支持emoji
    mysql_set_character_set(connection_, "utf8mb4");
    connected_ = true;
    return true;
}

/**
 * @brief 断开数据库连接
 */
void Database::disconnect() {
    if (connection_) {
        mysql_close(connection_);
        connection_ = nullptr;
    }
    connected_ = false;
}

/**
 * @brief 获取MySQL连接指针
 * @return MySQL连接指针
 */
MYSQL* Database::getConnection() {
    return connection_;
}

/**
 * @brief 检查是否已连接
 * @return 已连接返回true，否则返回false
 */
bool Database::isConnected() const {
    return connected_;
}

/**
 * @brief 执行SQL语句（INSERT、UPDATE、DELETE等）
 * @param sql SQL语句
 * @return 执行成功返回true，失败返回false
 */
bool Database::execute(const std::string& sql) {
    if (!connected_ || !connection_) {
        return false;
    }
    
    if (mysql_query(connection_, sql.c_str()) != 0) {
        fprintf(stderr, "MySQL execute error: %s\n", mysql_error(connection_));
        return false;
    }
    return true;
}

/**
 * @brief 执行查询SQL语句（SELECT）
 * @param sql SQL查询语句
 * @return 查询结果集，失败返回nullptr
 */
MYSQL_RES* Database::query(const std::string& sql) {
    if (!connected_ || !connection_) {
        return nullptr;
    }
    
    if (mysql_query(connection_, sql.c_str()) != 0) {
        fprintf(stderr, "MySQL query error: %s\n", mysql_error(connection_));
        return nullptr;
    }
    
    return mysql_store_result(connection_);
}

/**
 * @brief 释放查询结果集
 * @param res 查询结果集
 */
void Database::freeResult(MYSQL_RES* res) {
    if (res) {
        mysql_free_result(res);
    }
}
