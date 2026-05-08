/**
 * @file RedisClient.cpp
 * @brief Redis 客户端实现（基于 hiredis）
 */

#include "module/redis/RedisClient.h"
#include "module/Config.h"
#include <hiredis/hiredis.h>
#include <iostream>
#include <chrono>
#include <vector>

RedisClient::RedisClient() : ctx_(nullptr), connected_(false) {}

RedisClient::~RedisClient() {
    shutdown();
}

RedisClient& RedisClient::getInstance() {
    static RedisClient instance;
    return instance;
}

bool RedisClient::init(const std::string& host, int port) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connected_ && ctx_) return true;

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    ctx_ = redisConnectWithTimeout(host.c_str(), port, timeout);
    if (!ctx_ || ctx_->err) {
        if (ctx_) std::cerr << "Redis connect error: " << ctx_->errstr << std::endl;
        if (ctx_) redisFree(ctx_);
        ctx_ = nullptr;
        connected_ = false;
        return false;
    }
    connected_ = true;
    return true;
}

void RedisClient::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ctx_) {
        redisFree(ctx_);
        ctx_ = nullptr;
    }
    connected_ = false;
}

bool RedisClient::isConnected() const {
    return connected_ && ctx_;
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected()) return false;
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SET %s %s", key.c_str(), value.c_str());
    if (!reply) return false;
    bool ok = (reply->type != REDIS_REPLY_ERROR);
    freeReplyObject(reply);
    return ok;
}

bool RedisClient::setex(const std::string& key, int seconds, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected()) return false;
    redisReply* reply = (redisReply*)redisCommand(ctx_, "SETEX %s %d %s", key.c_str(), seconds, value.c_str());
    if (!reply) return false;
    bool ok = (reply->type != REDIS_REPLY_ERROR);
    freeReplyObject(reply);
    return ok;
}

std::string RedisClient::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected()) return std::string();
    redisReply* reply = (redisReply*)redisCommand(ctx_, "GET %s", key.c_str());
    if (!reply) return std::string();
    std::string val;
    if (reply->type == REDIS_REPLY_STRING && reply->str) val.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return val;
}

bool RedisClient::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected()) return false;
    redisReply* reply = (redisReply*)redisCommand(ctx_, "DEL %s", key.c_str());
    if (!reply) return false;
    bool ok = (reply->type != REDIS_REPLY_ERROR);
    freeReplyObject(reply);
    return ok;
}

bool RedisClient::lpush(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected()) return false;
    redisReply* reply = (redisReply*)redisCommand(ctx_, "LPUSH %s %s", key.c_str(), value.c_str());
    if (!reply) return false;
    bool ok = (reply->type != REDIS_REPLY_ERROR);
    freeReplyObject(reply);
    return ok;
}

std::vector<std::string> RedisClient::lrange(const std::string& key, int start, int stop) {
    std::vector<std::string> out;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!isConnected()) return out;
    redisReply* reply = (redisReply*)redisCommand(ctx_, "LRANGE %s %d %d", key.c_str(), start, stop);
    if (!reply) return out;
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (size_t i = 0; i < reply->elements; ++i) {
            redisReply* elem = reply->element[i];
            if (elem && elem->type == REDIS_REPLY_STRING) {
                out.emplace_back(elem->str, elem->len);
            }
        }
    }
    freeReplyObject(reply);
    return out;
}
