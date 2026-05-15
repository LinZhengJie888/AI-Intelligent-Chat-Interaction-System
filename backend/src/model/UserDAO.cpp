/**
 * @file UserDAO.cpp
 * @brief 用户数据访问对象实现
 */

#include "model/UserDAO.h"
#include "common/Util.h"
#include "module/redis/RedisClient.h"
#include <cstdio>
#include <sstream>
#include <vector>

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
    // 对所有字符串字段进行转义
    std::string safe_user_id = db_.escapeString(user.user_id);
    std::string safe_username = db_.escapeString(user.username);
    std::string safe_nickname = db_.escapeString(user.nickname);
    std::string safe_password = db_.escapeString(user.password);
    std::string safe_phone = db_.escapeString(user.phone);
    std::string safe_avatar_path = db_.escapeString(user.avatar_path);
    std::string safe_ai_nickname = db_.escapeString(user.ai_nickname);
    
    snprintf(sql, sizeof(sql),
            "INSERT INTO user (user_id, username, nickname, password, phone, "
            "avatar_path, ai_nickname, ai_tone, ai_priority) VALUES "
            "('%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d)",
            safe_user_id.c_str(), safe_username.c_str(),
            safe_nickname.c_str(), safe_password.c_str(),
            safe_phone.c_str(), safe_avatar_path.c_str(),
            safe_ai_nickname.c_str(), user.ai_tone, user.ai_priority);
    
    if (!db_.execute(sql)) {
        return false;
    }
    
    // 获取自增ID
    user.id = mysql_insert_id(db_.getConnection());
    // 写入 Redis 缓存（短期）
    try {
        std::ostringstream oss;
        oss << user.id << '\t' << user.user_id << '\t' << user.username << '\t' << user.nickname << '\t'
            << user.password << '\t' << user.phone << '\t' << user.avatar_path << '\t'
            << user.ai_nickname << '\t' << user.ai_tone << '\t' << user.ai_priority << '\t'
            << user.create_time << '\t' << user.update_time;
        std::string key = std::string("user:user_id:") + user.user_id;
        if (RedisClient::getInstance().isConnected()) {
            RedisClient::getInstance().setex(key, 3600, oss.str());
        }
    } catch (...) {}
    return true;
}

/**
 * @brief 更新用户记录
 * @param user 用户对象
 * @return 更新成功返回true，失败返回false
 */
bool UserDAO::update(const User& user) {
    char sql[2048];
    // 对所有字符串字段进行转义
    std::string safe_username = db_.escapeString(user.username);
    std::string safe_nickname = db_.escapeString(user.nickname);
    std::string safe_password = db_.escapeString(user.password);
    std::string safe_phone = db_.escapeString(user.phone);
    std::string safe_avatar_path = db_.escapeString(user.avatar_path);
    std::string safe_ai_nickname = db_.escapeString(user.ai_nickname);
    
    snprintf(sql, sizeof(sql),
            "UPDATE user SET username='%s', nickname='%s', password='%s', "
            "phone='%s', avatar_path='%s', ai_nickname='%s', ai_tone=%d, "
            "ai_priority=%d WHERE id=%lu",
            safe_username.c_str(), safe_nickname.c_str(), safe_password.c_str(),
            safe_phone.c_str(), safe_avatar_path.c_str(), safe_ai_nickname.c_str(),
            user.ai_tone, user.ai_priority, (unsigned long)user.id);
    bool ok = db_.execute(sql);
    if (ok) {
        try {
            std::ostringstream oss;
            oss << user.id << '\t' << user.user_id << '\t' << user.username << '\t' << user.nickname << '\t'
                << user.password << '\t' << user.phone << '\t' << user.avatar_path << '\t'
                << user.ai_nickname << '\t' << user.ai_tone << '\t' << user.ai_priority << '\t'
                << user.create_time << '\t' << user.update_time;
            std::string key = std::string("user:user_id:") + user.user_id;
            if (RedisClient::getInstance().isConnected()) {
                RedisClient::getInstance().setex(key, 3600, oss.str());
            }
        } catch (...) {}
    }
    return ok;
}

/**
 * @brief 删除用户记录
 * @param id 用户主键ID
 * @return 删除成功返回true，失败返回false
 */
bool UserDAO::remove(uint64_t id) {
    char sql[256];
    snprintf(sql, sizeof(sql), "DELETE FROM user WHERE id=%lu", (unsigned long)id);
    // 删除时同时删除 Redis 缓存（若存在）
    // 先查询 user_id
    char sel[256];
    snprintf(sel, sizeof(sel), "SELECT user_id FROM user WHERE id=%lu", (unsigned long)id);
    MYSQL_RES* res = db_.query(sel);
    std::string user_id;
    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row && row[0]) user_id = row[0];
        db_.freeResult(res);
    }

    bool ok = db_.execute(sql);
    if (ok && !user_id.empty()) {
        try {
            std::string key = std::string("user:user_id:") + user_id;
            if (RedisClient::getInstance().isConnected()) {
                RedisClient::getInstance().del(key);
            }
        } catch (...) {}
    }
    return ok;
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
    // 优先尝试从 Redis 读取缓存
    try {
        std::string key = std::string("user:user_id:") + user_id;
        if (RedisClient::getInstance().isConnected()) {
            std::string cached = RedisClient::getInstance().get(key);
            if (!cached.empty()) {
                // 解析缓存（使用 '\t' 分隔字段）
                std::vector<std::string> parts;
                std::istringstream iss(cached);
                std::string token;
                while (std::getline(iss, token, '\t')) parts.push_back(token);
                if (parts.size() >= 12) {
                    User* user = new User();
                    user->id = strtoull(parts[0].c_str(), nullptr, 10);
                    user->user_id = parts[1];
                    user->username = parts[2];
                    user->nickname = parts[3];
                    user->password = parts[4];
                    user->phone = parts[5];
                    user->avatar_path = parts[6];
                    user->ai_nickname = parts[7];
                    user->ai_tone = parts[8].empty() ? 0 : atoi(parts[8].c_str());
                    user->ai_priority = parts[9].empty() ? 0 : atoi(parts[9].c_str());
                    user->create_time = parts[10];
                    user->update_time = parts[11];
                    return user;
                }
            }
        }
    } catch (...) {
        // 忽略 Redis 错误，回退到数据库
    }

    // 回退到数据库查询
    char sql[256];
    std::string safe_user_id = db_.escapeString(user_id);
    snprintf(sql, sizeof(sql), "SELECT * FROM user WHERE user_id='%s'", safe_user_id.c_str());
    
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

    // 将查询结果写入 Redis 缓存
    try {
        std::ostringstream oss;
        oss << user->id << '\t' << user->user_id << '\t' << user->username << '\t' << user->nickname << '\t'
            << user->password << '\t' << user->phone << '\t' << user->avatar_path << '\t'
            << user->ai_nickname << '\t' << user->ai_tone << '\t' << user->ai_priority << '\t'
            << user->create_time << '\t' << user->update_time;
        std::string key = std::string("user:user_id:") + user->user_id;
        if (RedisClient::getInstance().isConnected()) {
            RedisClient::getInstance().setex(key, 3600, oss.str());
        }
    } catch (...) {
        // 忽略 Redis 写入错误
    }

    return user;
}

/**
 * @brief 根据手机号查询用户
 * @param phone 手机号
 * @return 用户对象指针，未找到返回nullptr（调用者负责释放内存）
 */
User* UserDAO::findByPhone(const std::string& phone) {
    char sql[256];
    std::string safe_phone = db_.escapeString(phone);
    snprintf(sql, sizeof(sql), "SELECT * FROM user WHERE phone='%s'", safe_phone.c_str());
    
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
