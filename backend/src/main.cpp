#include <iostream>
#include <string>
#include <functional>
#include <signal.h>
#include "reactor/TcpServer.h"
#include "reactor/EventLoop.h"
#include "reactor/Connection.h"
#include "reactor/Timestamp.h"

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

int main(int argc, char *argv[])
{
    std::string ip = "127.0.0.1";
    uint16_t port = 8080;
    
    if (argc == 3)
    {
        ip = argv[1];
        port = atoi(argv[2]);
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
