#include <iostream>
#include <string>
#include <functional>
#include <signal.h>
#include "reactor/TcpServer.h"
#include "reactor/EventLoop.h"
#include "reactor/Connection.h"
#include "reactor/Timestamp.h"
#include "module/Config.h"
#include "module/Database.h"
#include "model/User.h"
#include "model/UserDAO.h"
#include "common/Util.h"

TcpServer *server;

void Stop(int sig)
{
    std::cout << "Received signal " << sig << ", stopping server..." << std::endl;
    server->stop();
    delete server;
    std::cout << "Server stopped." << std::endl;
    exit(0);
}

void HandleNewConnection(spConnection conn)
{
    std::cout << Timestamp::now().tostring() << " New connection: fd=" << conn->fd() 
              << ", ip=" << conn->ip() << ", port=" << conn->port() << std::endl;
}

void HandleClose(spConnection conn)
{
    std::cout << Timestamp::now().tostring() << " Connection closed: fd=" << conn->fd() << std::endl;
}

void HandleMessage(spConnection conn, std::string& message)
{
    std::cout << Timestamp::now().tostring() << " Received message: " << message << std::endl;
    std::string reply = "Echo: " + message;
    conn->send(reply.data(), reply.size());
}

void HandleSendComplete(spConnection conn)
{
    std::cout << Timestamp::now().tostring() << " Send complete." << std::endl;
}

/**
 * @brief 测试数据库连接和model模块
 * @return 测试成功返回true，失败返回false
 */
bool TestDatabaseAndModel() {
    std::cout << "\n========== 开始测试数据库和model模块 ==========" << std::endl;
    
    // 1. 加载配置
    std::cout << "1. 加载配置文件..." << std::endl;
    Config& config = Config::getInstance();
    if (!config.loadFromFile("./config.ini")) {
        std::cout << "   警告: 无法加载配置文件，使用默认配置" << std::endl;
        config.saveToFile("./config.ini");
        std::cout << "   已创建默认配置文件 config.ini" << std::endl;
    }
    
    const DBConfig& db_config = config.getDBConfig();
    std::cout << "   数据库配置: " << db_config.user << "@" << db_config.host << ":" << db_config.port << "/" << db_config.dbname << std::endl;
    
    // 2. 连接数据库
    std::cout << "\n2. 连接数据库..." << std::endl;
    Database db;
    if (!db.connect(db_config.host, db_config.port,
                     db_config.user, db_config.password, db_config.dbname)) {
        std::cout << "   错误: 数据库连接失败！" << std::endl;
        std::cout << "   请检查 config.ini 中的数据库配置是否正确" << std::endl;
        return false;
    }
    std::cout << "   数据库连接成功！" << std::endl;
    
    // 3. 测试UserDAO
    std::cout << "\n3. 测试UserDAO..." << std::endl;
    UserDAO user_dao(db);
    
    // 4. 尝试删除已存在的测试用户
    std::cout << "   清理测试数据..." << std::endl;
    User* existing_user = user_dao.findByUserId("test");
    if (existing_user != nullptr) {
        user_dao.remove(existing_user->id);
        delete existing_user;
        std::cout << "   已删除旧的测试用户" << std::endl;
    }
    
    // 5. 创建测试用户
    std::cout << "\n5. 创建测试用户..." << std::endl;
    User test_user;
    test_user.user_id = "test";
    test_user.username = "测试用户";
    test_user.nickname = "测试昵称";
    test_user.password = util::md5("123456");
    test_user.phone = "13800138000";
    test_user.avatar_path = "";
    test_user.ai_nickname = "AI助手";
    test_user.ai_tone = 0;
    test_user.ai_priority = 0;
    
    if (user_dao.insert(test_user)) {
        std::cout << "   测试用户创建成功！用户ID: " << test_user.id << std::endl;
    } else {
        std::cout << "   错误: 测试用户创建失败！" << std::endl;
        return false;
    }
    
    // 6. 查询测试用户
    std::cout << "\n6. 查询测试用户..." << std::endl;
    User* queried_user = user_dao.findByUserId("test");
    if (queried_user != nullptr) {
        std::cout << "   查询成功！" << std::endl;
        std::cout << "   用户ID: " << queried_user->user_id << std::endl;
        std::cout << "   用户名: " << queried_user->username << std::endl;
        std::cout << "   昵称: " << queried_user->nickname << std::endl;
        std::cout << "   密码(MD5): " << queried_user->password << std::endl;
        delete queried_user;
    } else {
        std::cout << "   错误: 查询测试用户失败！" << std::endl;
        return false;
    }
    
    std::cout << "\n========== 数据库和model模块测试完成 ==========\n" << std::endl;
    return true;
}

int main(int argc, char *argv[])
{
    std::string ip = "127.0.0.1";
    uint16_t port = 8080;
    
    if (argc == 3)
    {
        ip = argv[1];
        port = atoi(argv[2]);
    }
    
    // 测试数据库和model模块
    if (!TestDatabaseAndModel()) {
        std::cout << "测试失败，程序退出" << std::endl;
        return -1;
    }
    
    signal(SIGTERM, Stop);
    signal(SIGINT, Stop);
    
    std::cout << "Starting echo server on " << ip << ":" << port << "..." << std::endl;
    
    server = new TcpServer(ip, port, 3);
    
    server->setnewconnectioncb(HandleNewConnection);
    server->setcloseconnectioncb(HandleClose);
    server->setonmessagecb(HandleMessage);
    server->setsendcompletecb(HandleSendComplete);
    
    server->start();
    
    return 0;
}
