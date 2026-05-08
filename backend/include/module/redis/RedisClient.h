/**
 * @file RedisClient.h
 * @brief 简单的 Redis 客户端封装（基于 hiredis）
 */
#pragma once

#include <string>
#include <mutex>
#include <vector>

struct redisContext;

class RedisClient {
public:
    static RedisClient& getInstance();

    bool init(const std::string& host = "127.0.0.1", int port = 6379);
    void shutdown();

    bool isConnected() const;

    bool set(const std::string& key, const std::string& value);
    bool setex(const std::string& key, int seconds, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    // list operations
    bool lpush(const std::string& key, const std::string& value);
    std::vector<std::string> lrange(const std::string& key, int start, int stop);

private:
    RedisClient();
    ~RedisClient();

    redisContext* ctx_;
    bool connected_;
    mutable std::mutex mutex_;
};
