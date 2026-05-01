/**
 * @file Config.h
 * @brief 配置管理类
 * 
 * 采用单例模式，统一管理数据库、AI、服务器等配置信息。
 */

#pragma once

#include <string>

/**
 * @struct DBConfig
 * @brief 数据库配置结构体
 */
struct DBConfig {
    std::string host;     ///< 数据库主机地址
    int port;             ///< 数据库端口
    std::string user;     ///< 数据库用户名
    std::string password; ///< 数据库密码
    std::string dbname;   ///< 数据库名称
};

/**
 * @struct AIConfig
 * @brief AI配置结构体
 */
struct AIConfig {
    std::string api_url;          ///< AI API地址
    std::string api_key;          ///< AI API密钥
    std::string default_nickname; ///< AI默认昵称
    int default_tone;             ///< AI默认语气
    int default_priority;         ///< AI默认响应优先级
};

/**
 * @struct ServerConfig
 * @brief 服务器配置结构体
 */
struct ServerConfig {
    int port;                  ///< 服务器端口
    int thread_pool_size;      ///< 线程池大小
    std::string avatar_storage_path; ///< 头像存储路径
};

/**
 * @class Config
 * @brief 配置管理类（单例模式）
 */
class Config {
public:
    /**
     * @brief 获取单例实例
     * @return Config单例引用
     */
    static Config& getInstance();
    
    /**
     * @brief 获取数据库配置
     * @return 数据库配置常量引用
     */
    const DBConfig& getDBConfig() const;
    
    /**
     * @brief 获取AI配置
     * @return AI配置常量引用
     */
    const AIConfig& getAIConfig() const;
    
    /**
     * @brief 获取服务器配置
     * @return 服务器配置常量引用
     */
    const ServerConfig& getServerConfig() const;
    
    /**
     * @brief 设置数据库配置
     * @param config 数据库配置
     */
    void setDBConfig(const DBConfig& config);
    
    /**
     * @brief 设置AI配置
     * @param config AI配置
     */
    void setAIConfig(const AIConfig& config);
    
    /**
     * @brief 设置服务器配置
     * @param config 服务器配置
     */
    void setServerConfig(const ServerConfig& config);
    
    /**
     * @brief 从文件加载配置
     * @param filename 配置文件路径
     * @return 加载成功返回true，失败返回false
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief 保存配置到文件
     * @param filename 配置文件路径
     * @return 保存成功返回true，失败返回false
     */
    bool saveToFile(const std::string& filename);
    
private:
    /**
     * @brief 私有构造函数，防止外部实例化
     */
    Config();
    
    /**
     * @brief 析构函数
     */
    ~Config() = default;
    
    /**
     * @brief 禁用拷贝构造函数
     */
    Config(const Config&) = delete;
    
    /**
     * @brief 禁用赋值运算符
     */
    Config& operator=(const Config&) = delete;
    
    DBConfig db_config_;      ///< 数据库配置
    AIConfig ai_config_;      ///< AI配置
    ServerConfig server_config_; ///< 服务器配置
};
