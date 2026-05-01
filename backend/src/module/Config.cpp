/**
 * @file Config.cpp
 * @brief 配置管理类实现
 */

#include "module/Config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>

/**
 * @brief 去除字符串首尾的空白字符
 * @param str 输入字符串
 * @return 去除空白后的字符串
 */
static std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return str.substr(start, end - start + 1);
}

/**
 * @brief 构造函数，初始化默认配置
 */
Config::Config() {
    // 数据库默认配置
    db_config_.host = "127.0.0.1";
    db_config_.port = 3306;
    db_config_.user = "root";
    db_config_.password = "123456";
    db_config_.dbname = "ai_chat_system";
    
    // AI默认配置
    ai_config_.api_url = "";
    ai_config_.api_key = "";
    ai_config_.default_nickname = "AI助手";
    ai_config_.default_tone = 0;
    ai_config_.default_priority = 0;
    
    // 服务器默认配置
    server_config_.port = 8080;
    server_config_.thread_pool_size = 4;
    server_config_.avatar_storage_path = "./backend/static/avatars";
}

/**
 * @brief 获取单例实例
 * @return Config单例引用
 */
Config& Config::getInstance() {
    static Config instance;
    return instance;
}

/**
 * @brief 获取数据库配置
 * @return 数据库配置常量引用
 */
const DBConfig& Config::getDBConfig() const {
    return db_config_;
}

/**
 * @brief 获取AI配置
 * @return AI配置常量引用
 */
const AIConfig& Config::getAIConfig() const {
    return ai_config_;
}

/**
 * @brief 获取服务器配置
 * @return 服务器配置常量引用
 */
const ServerConfig& Config::getServerConfig() const {
    return server_config_;
}

/**
 * @brief 设置数据库配置
 * @param config 数据库配置
 */
void Config::setDBConfig(const DBConfig& config) {
    db_config_ = config;
}

/**
 * @brief 设置AI配置
 * @param config AI配置
 */
void Config::setAIConfig(const AIConfig& config) {
    ai_config_ = config;
}

/**
 * @brief 设置服务器配置
 * @param config 服务器配置
 */
void Config::setServerConfig(const ServerConfig& config) {
    server_config_ = config;
}

/**
 * @brief 从文件加载配置
 * @param filename 配置文件路径
 * @return 加载成功返回true，失败返回false
 */
bool Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string current_section;
    
    while (std::getline(file, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.size() - 2);
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            
            if (current_section == "database") {
                if (key == "host") db_config_.host = value;
                else if (key == "port") db_config_.port = std::stoi(value);
                else if (key == "user") db_config_.user = value;
                else if (key == "password") db_config_.password = value;
                else if (key == "dbname") db_config_.dbname = value;
            } else if (current_section == "ai") {
                if (key == "api_url") ai_config_.api_url = value;
                else if (key == "api_key") ai_config_.api_key = value;
                else if (key == "default_nickname") ai_config_.default_nickname = value;
                else if (key == "default_tone") ai_config_.default_tone = std::stoi(value);
                else if (key == "default_priority") ai_config_.default_priority = std::stoi(value);
            } else if (current_section == "server") {
                if (key == "port") server_config_.port = std::stoi(value);
                else if (key == "thread_pool_size") server_config_.thread_pool_size = std::stoi(value);
                else if (key == "avatar_storage_path") server_config_.avatar_storage_path = value;
            }
        }
    }
    
    file.close();
    return true;
}

/**
 * @brief 保存配置到文件
 * @param filename 配置文件路径
 * @return 保存成功返回true，失败返回false
 */
bool Config::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# AI智能聊天互动系统配置文件\n\n";
    
    file << "[database]\n";
    file << "host = " << db_config_.host << "\n";
    file << "port = " << db_config_.port << "\n";
    file << "user = " << db_config_.user << "\n";
    file << "password = " << db_config_.password << "\n";
    file << "dbname = " << db_config_.dbname << "\n\n";
    
    file << "[ai]\n";
    file << "api_url = " << ai_config_.api_url << "\n";
    file << "api_key = " << ai_config_.api_key << "\n";
    file << "default_nickname = " << ai_config_.default_nickname << "\n";
    file << "default_tone = " << ai_config_.default_tone << "\n";
    file << "default_priority = " << ai_config_.default_priority << "\n\n";
    
    file << "[server]\n";
    file << "port = " << server_config_.port << "\n";
    file << "thread_pool_size = " << server_config_.thread_pool_size << "\n";
    file << "avatar_storage_path = " << server_config_.avatar_storage_path << "\n";
    
    file.close();
    return true;
}
