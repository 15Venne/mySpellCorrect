
#ifndef __TCPHEAD_H__
#define __TCPHEAD_H__


#include"mylog.h"
#include"threadpool.h"
#include"Noncopy.h"

#include<arpa/inet.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/epoll.h>
#include<sys/eventfd.h>

#include<iostream>
#include<string>
#include<memory>
#include<functional>
#include<sstream>
#include<map>
#include<vector>

using std::string;
using std::cin;
using std::cout;
using std::endl;
using std::function;
using std::shared_ptr;
using std::vector;
using std::map;

//----------------------------------------------------------------------------
//InetAddress ip地址，端口
class InetAddress
{
public:
    explicit
    InetAddress(unsigned short);//1
    InetAddress(const string &, unsigned short);//2
    InetAddress(const struct sockaddr_in &);//3

    string ip() const;//4
    unsigned short port() const;//5
    struct sockaddr_in *getInetAddressPtr()
    {
        return &_addr;
    }

private:
    struct sockaddr_in _addr;
};//

InetAddress::InetAddress(unsigned short port)
{
    ::memset(&_addr, 0, sizeof(struct sockaddr_in));
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = INADDR_ANY;//本机地址
}//1
InetAddress::InetAddress(const string &ip, unsigned short port)
{
    ::memset(&_addr, 0, sizeof(struct sockaddr_in));
    _addr.sin_family = AF_INET;
    _addr.sin_port = htons(port);
    _addr.sin_addr.s_addr = inet_addr(ip.c_str());
}//2
InetAddress::InetAddress(const struct sockaddr_in &addr)
: _addr(addr)
{  }//3

string InetAddress::ip() const
{
    return string(::inet_ntoa(_addr.sin_addr));
}//4

unsigned short InetAddress::port() const
{
    return ntohs(_addr.sin_port);
}//5
//--------------------------------------------------------------------------------
//Socket 套接字
class Socket
{
public:
    Socket()
    {
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if(-1 == _fd)
        {
            perror("socket");
        }
    }
    explicit
    Socket(int fd)
    : _fd(fd)
    {  }
    int fd() const
    {
        return _fd;
    }
    void shutdownWrite()
    {
        ::shutdown(_fd, SHUT_WR);
    }
    ~Socket()
    {
        ::close(_fd);
    }
private:
    int _fd;
};//
//---------------------------------------------------------------------------------
//SocketIO
class SocketIO
{
public:
    explicit
    SocketIO(int fd)
    : _fd(fd)
    {  }
    int readn(char*, int);//1
    int writen(const char*, int);//2
    int readline(char*, int);//3
private:
    int recvPeek(char*, int);//4
private:
    int _fd;
};//

int SocketIO::readn(char *buf, int len)
{
    int total = 0;
    int ret;
    char *p = buf;
    while(total < len)
    {
        ret = ::read(_fd, p + total, len - total);
        if(-1 == ret)
        {
            if(errno == EINTR)
            {
                continue;
            }else{
                perror("read");
                break;
            }
        }else if( 0 == ret ){
            break;   
        }else{
            total += ret;
        }
    }
    return total;
}//1

int SocketIO::writen(const char *buf, int len)
{
    int total = 0;
    int ret;
    const char *p = buf;
    while(total < len)
    {
        ret = ::write(_fd, p + total, len - total);
        if(-1 == ret)
        {
            if(errno == EINTR)
            {
                continue;
            }else{
                perror("read");
                break;
            }
        }else if( 0 == ret ){
            break;   
        }else{
            total += ret;
        }
    }
    return total;
}//2

int SocketIO::readline(char *buf, int maxLen)
{
    int left = maxLen - 1;
    char *p = buf;
    int ret;
    int total = 0;
    while(left > 0)
    {
        ret = recvPeek(p, left);
        for(int i = 0; i < ret; ++i)
        {
            if(p[i] == '\n')
            {
                readn(p, i + 1);
                total += i+1;
                p += i+1;
                *p = '\0';
                return total;
            }
        }
        //没有发现'\n'
        readn(p, ret);
        left -= ret;
        total += ret;
        p += ret;
    }
    *p = '\0';
    return total;
}//3

int SocketIO::recvPeek(char *buf, int len)
{
    int ret;
    do{
        ret = ::recv(_fd, buf, len, MSG_PEEK);
    }while(ret == -1 && errno == EINTR);
    return ret;
}//4
//---------------------------------------------------------------------------------
//Acceptor ----listen,bind,accept
class Acceptor
{
public:
    Acceptor(unsigned short);//1
    Acceptor(const string &, unsigned short);//2
    void ready();//3
    int accept();//4
    int fd() const
    {
        return _socketFd.fd();
    }
private:
    void setReuseAddr(bool);//5
    void setReusePort(bool);//6
    void bind();//7
    void listen();//8
private:
    InetAddress _addr;
    Socket _socketFd;
};//

Acceptor::Acceptor(unsigned short port)
: _addr(port)
, _socketFd()
{  }//1

Acceptor::Acceptor(const string &ip, unsigned short port)
: _addr(ip, port)
, _socketFd()
{  }//2

void Acceptor::ready()
{
    setReuseAddr(true);
    setReusePort(true);
    bind();
    listen();
}//3

int Acceptor::accept()
{
    int peerfd = ::accept(_socketFd.fd(), NULL, NULL);
    if(-1 == peerfd)
    {
        perror("accept");
    }
    return peerfd;
}//4
void Acceptor::setReuseAddr(bool on)
{
    int one = on;
    int ret;
    ret = setsockopt(_socketFd.fd(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(ret < 0)
    {
        perror("setReuseAddr::setsockopt");
    }
}//5
void Acceptor::setReusePort(bool on)
{
    int one = on;
    int ret;
    ret = setsockopt(_socketFd.fd(), SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
    if(ret < 0)
    {
        perror("setReusePort::setsockopt");
    }
}//6
void Acceptor::bind()
{
    int ret;
    ret = ::bind(_socketFd.fd(), (struct sockaddr*)_addr.getInetAddressPtr(), sizeof(struct sockaddr));
    if(-1 == ret)
    {
        perror("bind");
    }
}//7
void Acceptor::listen()
{
    int ret;
    ret = ::listen(_socketFd.fd(), 10);
    if(-1 == ret)
    {
        perror("listen");
    }
}//8
//-------------------------------------------------------------------------------------

class EventLoop;
//TcpConnection

class TcpConnection
: public Noncopyable
, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(int, EventLoop*);//1
    string receive();//2
    void send(const string&);//3
    void sendInLoop(const string &);//4

    string toString() const;//5
    void shutdown();//6

    void setConnectionCallback(const function<void(shared_ptr<TcpConnection>)> &cb)
    {
        _cbConnection = std::move(cb);
    }
    void setMessageCallback(const function<void(shared_ptr<TcpConnection>)> &cb)
    {
        _cbMessage = std::move(cb);
    }
    void setCloseCallback(const function<void(shared_ptr<TcpConnection>)> &cb)
    {
        _cbClose = std::move(cb);
    }
    void handleConnectionCallback()
    {
        if(_cbConnection)
        {
            _cbConnection(shared_from_this());
        }
    }
    void handleMessageCallback()
    {
        if(_cbMessage)
        {
            _cbMessage(shared_from_this());
        }
    }
    void handleCloseCallback()
    {
        if(_cbClose)
        {
            _cbClose(shared_from_this());
        }
    }
private:
    InetAddress getLocalAddr(int fd)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(struct sockaddr);
        int ret;
        ret = getsockname(fd, (struct sockaddr*)&addr, &len);
        if(-1 == ret)
        {
            perror("getsockname");
        }
        return InetAddress(addr);
    }

    InetAddress getPeerAddr(int fd)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(struct sockaddr);
        int ret;
        ret = getpeername(fd, (struct sockaddr*)&addr, &len);
        if(-1 == ret)
        {
            perror("getsockname");
        }
        return InetAddress(addr);
    }
private:
    Socket _socket;
    SocketIO _socketIo;
    InetAddress _localAddr;
    InetAddress _peerAddr;
    bool _isShutdownWrite;
    EventLoop *_ploop;

    function<void(shared_ptr<TcpConnection>)> _cbConnection;
    function<void(shared_ptr<TcpConnection>)> _cbMessage;
    function<void(shared_ptr<TcpConnection>)> _cbClose;
};//
class EventLoop
{
public:
    EventLoop(Acceptor &acceptor);//1
    void loop();//2
    void unloop();//3
    void runInLoop(function<void()> &&cb);//4

    void setConnectionCallback(function<void(const shared_ptr<TcpConnection>)> &&cb)
    {
        _cbConnection = std::move(cb);
    }
    void setMessageCallback(function<void(const shared_ptr<TcpConnection>)> &&cb)
    {
        _cbMessage = std::move(cb);
    }
    void setCloseCallback(function<void(const shared_ptr<TcpConnection>)> &&cb)
    {
        _cbClose = std::move(cb);
    }
private:
    void waitEpollFd();//5
    void handleNewConnection();//6
    void handleMessage(int);//7
    void handleRead();//8
    void wakeup();//9
    int createEpollFd();//10
    int createEventFd();//11
    void doPendingFunctors();//12

    void addEpollFdRead(int);//13
    void delEpollFdRead(int);//14
    bool isConnectionClosed(int);//15

private:
    int _efd;
    int _eventFd;
    Acceptor &_acceptor;
    vector<struct epoll_event> _events;
    map<int, shared_ptr<TcpConnection>> _conns;
    bool _isLooping;

    MutexLock _mutex;
    vector<function<void()>> _pendingFunctors;

    function<void(const shared_ptr<TcpConnection>)> _cbConnection;
    function<void(const shared_ptr<TcpConnection>)> _cbMessage;
    function<void(const shared_ptr<TcpConnection>)> _cbClose;
};//
TcpConnection::TcpConnection(int fd, EventLoop *ploop)
: _socket(fd)
, _socketIo(fd)
, _localAddr(getLocalAddr(fd))
, _peerAddr(getPeerAddr(fd))
, _isShutdownWrite(false)
, _ploop(ploop)
{  }//1

string TcpConnection::receive()
{
    char buf[65536] = {0};
    _socketIo.readline(buf, sizeof(buf));
    return string(buf);
}//2

void TcpConnection::send(const string &msg)
{
    _socketIo.writen(msg.c_str(), msg.size());
}//3

void TcpConnection::sendInLoop(const string &msg)
{
    _ploop->runInLoop(std::bind(&TcpConnection::send, this, msg));   
}//4

string TcpConnection::toString() const
{
    std::ostringstream oss;
    oss << _localAddr.ip() << ":" << _localAddr.port() << "-->"
        << _peerAddr.ip() << ":" << _peerAddr.port();
    return oss.str();
}//5
void TcpConnection::shutdown()
{
    _isShutdownWrite = true;
    _socket.shutdownWrite();
}//6
//-------------------------------------------------------------------------
//EventLoop -------------- epoll


EventLoop::EventLoop(Acceptor &acceptor)
: _efd(createEpollFd())
, _eventFd(createEventFd())
, _acceptor(acceptor)
, _events(1024)
, _conns()
, _isLooping(false)
{
    addEpollFdRead(_acceptor.fd());
    addEpollFdRead(_eventFd);
}//1

void EventLoop::loop()
{
    _isLooping = true;
    while(_isLooping)
    {
        waitEpollFd();
    }
}//2
void EventLoop::unloop()
{
    _isLooping = false;
}//3
void EventLoop::runInLoop(function<void()> &&cb)
{
    {
    MutexLockGuard autoLock(_mutex);
    _pendingFunctors.push_back(std::move(cb));
    } 
    wakeup();
}//4

void EventLoop::waitEpollFd()
{
    int readyNum;
    do{
        readyNum = epoll_wait(_efd, &*_events.begin(), _events.size(), -1);
    }while(readyNum == -1 && errno == EINTR);
    if(readyNum == -1)
    {
        perror("epoll_wait");
        return;
    }else{
        if(readyNum == _events.size())
        {
            _events.resize(2 *readyNum);
        }
        for(int i = 0; i < readyNum; ++i)
        {
            int fd = _events[i].data.fd;
            if(fd == _acceptor.fd())//处理新连接
            {
                if(_events[i].events & EPOLLIN)
                {
                    handleNewConnection();
                }
            }else if(fd == _eventFd){ //处理线程消息
                if(_events[i].events & EPOLLIN)
                {
                    handleRead();
                    //cout << ">>before doPendingFunctors()" << endl;
                    doPendingFunctors();
                    //cout << ">>after doPendingFunctors()" << endl;
                }
            }else{//处理客户消息
                if(_events[i].events & EPOLLIN)
                {
                    handleMessage(fd);
                }
            }
        }
    }
}//5

void EventLoop::handleNewConnection()
{
    int peerfd = _acceptor.accept();
    addEpollFdRead(peerfd);
    shared_ptr<TcpConnection> conn(new TcpConnection(peerfd,this));
    conn->setConnectionCallback(_cbConnection);
    conn->setMessageCallback(_cbMessage);
    conn->setCloseCallback(_cbClose);
    _conns.insert(std::make_pair(peerfd, conn));
    
    conn->handleConnectionCallback();
}//6

void EventLoop::handleMessage(int fd)
{
    bool isClosed = isConnectionClosed(fd);
    auto iter = _conns.find(fd);
    if(iter != _conns.end())
    {
        if(isClosed)
        {
            delEpollFdRead(fd);
            iter->second->handleCloseCallback();
            _conns.erase(iter);
        }else{
            iter->second->handleMessageCallback();
        }
    }
}//7
void EventLoop::handleRead()
{
    uint64_t howmany;
    int ret = ::read(_eventFd, &howmany, sizeof(howmany));
    if(ret != sizeof(howmany))
    {
        perror("read");
    }
}//8
void EventLoop::wakeup()
{
    uint64_t one = 1;
    int ret = ::write(_eventFd, &one, sizeof(one));
    if(ret != sizeof(one))
    {
        perror("write");
    }
}//9
int EventLoop::createEpollFd()
{
    int ret = ::epoll_create1(0);
    if(-1 == ret)
    {
        perror("epoll_create1");
    }
    return ret;
}//10

int EventLoop::createEventFd()
{
    int ret = ::eventfd(0, 0);
    if(-1 == ret)
    {
        perror("eventfd");
    }
    return ret;
}//11
void EventLoop::doPendingFunctors()
{
    vector<function<void()>> tmp;
    {
    MutexLockGuard autoLock(_mutex);
    tmp.swap(_pendingFunctors);
    } 
    for(auto & f : tmp)
    {
        f();
    }
}//12

void EventLoop::addEpollFdRead(int fd)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    evt.events = EPOLLIN;
    int ret;
    ret = epoll_ctl(_efd, EPOLL_CTL_ADD, fd, &evt);
    if(-1 == ret)
    {
        perror("epoll_ctl");
    }
}//13
void EventLoop::delEpollFdRead(int fd)
{
    struct epoll_event evt;
    evt.data.fd = fd;
    int ret; 
    ret = epoll_ctl(_efd, EPOLL_CTL_DEL, fd, &evt);
    if(-1 == ret)
    {
        perror("epoll_ctl");
    }
}//14
bool EventLoop::isConnectionClosed(int fd)
{
    int ret;
    do{
        char buf[1024];
        ret = recv(fd, buf, sizeof(buf), MSG_PEEK);
    }while(ret == 1 && errno == EINTR);

    return (ret == 0);
}//15
//--------------------------------------------------------------------------------------
//TcpServer
class TcpServer
{
public:
    TcpServer(const string &ip, unsigned short port)
    : _acceptor(ip, port)
    , _loop(_acceptor)
    {  }
    void start();//1
    void setConnectionCallback(function<void(const shared_ptr<TcpConnection>)> &&cb)
    {
        _loop.setConnectionCallback(std::move(cb));
    }
    void setMessageCallback(function<void(const shared_ptr<TcpConnection>)> &&cb)
    {
        _loop.setMessageCallback(std::move(cb));
    }
    void setCloseCallback(function<void(const shared_ptr<TcpConnection>)> &&cb)
    {
        _loop.setCloseCallback(std::move(cb));
    }
private:
    Acceptor _acceptor;
    EventLoop _loop;
};//
void TcpServer::start()
{
    _acceptor.ready();
    _loop.loop();
}//1















#endif


