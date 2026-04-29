#include "Acceptor.h"

Acceptor::Acceptor(EventLoop* loop,const std::string &ip,const uint16_t port)
               :loop_(loop),servsock_(createnonblocking()),acceptchannel_(loop_,servsock_.fd())
{
    InetAddress servaddr(ip,port);             // 服务端的地址和协议。
    servsock_.setreuseaddr(true);
    servsock_.settcpnodelay(true);
    servsock_.setreuseport(true);
    servsock_.setkeepalive(true);
    servsock_.bind(servaddr);
    servsock_.listen();

    acceptchannel_.setreadcallback(std::bind(&Acceptor::newconnection,this));
    acceptchannel_.enablereading();       // 让epoll_wait()监视servchannel的读事件。 
}

Acceptor::~Acceptor()
{
}

// 处理新客户端连接请求。
void Acceptor::newconnection()    
{
    InetAddress clientaddr;             // 客户端的地址和协议。
    int cfd = servsock_.accept(clientaddr);
    if (cfd < 0) return; // accept 出错，直接返回

    std::unique_ptr<Socket> clientsock(new Socket(cfd));
    clientsock->setipport(clientaddr.ip(),clientaddr.port());

    if (newconnectioncb_)
        newconnectioncb_(std::move(clientsock));      // 回调TcpServer::newconnection()。
} 

void Acceptor::setnewconnectioncb(std::function<void(std::unique_ptr<Socket>)> fn)       // 设置处理新客户端连接请求的回调函数。
{
    newconnectioncb_=fn;
}
