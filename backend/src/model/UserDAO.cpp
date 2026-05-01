/**
 * @file UserDAO.cpp
 * @brief 用户数据访问对象实现
 */

#include "model/UserDAO.h"
#include "common/Util.h"
#include <cstdio>

/**
 * @brief 构造函数
 * @param db 数据库连接引用
 */
UserDAO::UserDAO(Database& db) : db_(db) {}

/**
 * @brief 插入用户记录
 * @param user 用户对象（插入后会自动填充id）
 * @return 插入成功返回true，失败返回false
 */
bool UserDAO::insert(User& user) {
    char sql[2048];
    snprintf(sql, sizeof(sql),
            "INSERT INTO user (user_id, username, nickname, password, phone, "
            "avatar_path, ai_nickname, ai_tone, ai_priority) VALUES "
            "('%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d)",
            user.user_id.c_str(), user.username.c_str(),
            user.nickname.c_str(), user.password.c_str(),
            user.phone.c_str(), user.avatar_path.c_str(),
            user.ai_nickname.c_str(), user.ai_tone, user.ai_priority);
    
    if (!db_.execute(sql)) {
        return false;
    }
    
    // 获取自增ID
    user.id = mysql_insert_id(db_.getConnection());
    return true;
}

/**
 * @brief 更新用户记录
 * @param user 用户对象
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::update(const User& user) {
    char sql[2048];
    snprintf(sql, sizeof(sql),
            "UPDATE user SET username='%s', nickname='%s', password='%s', "
            "phone='%s', avatar_path='%s', ai_nickname='%s', ai_tone=%d, "
            "ai_priority=%d WHERE id=%lu",
            user.username.c_str(), user.nickname.c_str(), user.password.c_str(),
            user.phone.c_str(), user.avatar_path.c_str(), user.ai_nickname.c_str(),
            user.ai_tone, user.ai_priority, (unsigned long)user.id);
    
    return db_.execute(sql);
}

/**
 * @brief 删除用户记录
 * @param id 用户主键ID
 * @return 删除成功返回true，失败返回false
 */
bool UserDAO::remove(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM user WHERE id=%lu", (unsigned long)id);
    return db_.execute(sql);
}

/**
 * @brief 根据主键ID查询用户
 * @param id 用户主键ID
 * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
 */
User* UserDAO::findById(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM user WHERE id=%lu", (unsigned long)id);
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    User* user = new User();
    user->id = strtoull(row[0], nullptr, 10);
    user->user_id = row[1] ? row[1] : "";
    user->username = row[2] ? row[2] : "";
    user->nickname = row[3] ? row[3] : "";
    user->password = row[4] ? row[4] : "";
    user->phone = row[5] ? row[5] : "";
    user->avatar_path = row[6] ? row[6] : "";
    user->ai_nickname = row[7] ? row[7] : "";
    user->ai_tone = row[8] ? atoi(row[8]) : 0;
    user->ai_priority = row[9] ? atoi(row[9]) : 0;
    user->create_time = row[10] ? row[10] : "";
    user->update_time = row[11] ? row[11] : "";
    
    db_.freeResult(res);
    return user;
}

/**
 * @brief 根据用户ID查询用户
 * @param user_id 用户ID
 * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
 */
User* UserDAO::findByUserId(const std::string& user_id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM user WHERE user_id='%s'", user_id.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    User* user = new User();
    user->id = strtoull(row[0], nullptr, 10);
    user->user_id = row[1] ? row[1] : "";
    user->username = row[2] ? row[2] : "";
    user->nickname = row[3] ? row[3] : "";
    user->password = row[4] ? row[4] : "";
    user->phone = row[5] ? row[5] : "";
    user->avatar_path = row[6] ? row[6] : "";
    user->ai_nickname = row[7] ? row[7] : "";
    user->ai_tone = row[8] ? atoi(row[8]) : 0;
    user->ai_priority = row[9] ? atoi(row[9]) : 0;
    user->create_time = row[10] ? row[10] : "";
    user->update_time = row[11] ? row[11] : "";
    
    db_.freeResult(res);
    return user;
}

/**
 * @brief 根据手机号查询用户
 * @param phone 手机号
 * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
 */
User* UserDAO::findByPhone(const std::string& phone) {
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT * FROM user WHERE phone='%s'", phone.c_str());
    
    MYSQL_RES* res = db_.query(sql);
    if (!res) return nullptr;
    
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) {
        db_.freeResult(res);
        return nullptr;
    }
    
    User* user = new User();
    user->id = strtoull(row[0], nullptr, 10);
    user->user_id = row[1] ? row[1] : "";
    user->username = row[2] ? row[2] : "";
    user->nickname = row[3] ? row[3] : "";
    user->password = row[4] ? row[4] : "";
    user->phone = row[5] ? row[5] : "";
    user->avatar_path = row[6] ? row[6] : "";
    user->ai_nickname = row[7] ? row[7] : "";
    user->ai_tone = row[8] ? atoi(row[8]) : 0;
    user->ai_priority = row[9] ? atoi(row[9]) : 0;
    user->create_time = row[10] ? row[10] : "";
    user->update_time = row[11] ? row[11] : "";
    
    db_.freeResult(res);
    return user;
}

/**
 * @brief 查询所有用户
 * @return 用户列表
 */
std::vector<User> UserDAO::findAll() {
    std::vector<User> users;
    MYSQL_RES* res = db_.query("SELECT * FROM user");
    if (!res) return users;
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)) != nullptr) {
        User user;
        user.id = strtoull(row[0], nullptr, 10);
        user.user_id = row[1] ? row[1] : "";
        user.username = row[2] ? row[2] : "";
        user.nickname = row[3] ? row[3] : "";
        user.password = row[4] ? row[4] : "";
        user.phone = row[5] ? row[5] : "";
        user.avatar_path = row[6] ? row[6] : "";
        user.ai_nickname = row[7] ? row[7] : "";
        user.ai_tone = row[8] ? atoi(row[8]) : 0;
        user.ai_priority = row[9] ? atoi(row[9]) : 0;
        user.create_time = row[10] ? row[10] : "";
        user.update_time = row[11] ? row[11] : "";
        users.push_back(user);
    }
    
    db_.freeResult(res);
    return users;
}

/**
 * @brief 验证密码
 * @param user_id 用户ID
 * @param password 明文密码
 * @return 密码正确返回true，否则返回false
 */
bool UserDAO::verifyPassword(const std::string& user_id, const std::string& password) {
    User* user = findByUserId(user_id);
    if (!user) return false;
    
    std::string hashed = util::md5(password);
    bool result = (user->password == hashed);
    delete user;
    return result;
}
