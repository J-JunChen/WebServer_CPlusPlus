#include "webserver.h"
using namespace std;

WebServer::WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, /* 端口，ET模式，timeoutMs，优雅退出 */
        int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName, /* Mysql配置 */
        int connPoolNum, int threadNum, /* 连接池数量，线程池数量 */
        bool openLog, int logLevel, int logQueSize /* 日志开关，日志等级，日志异步队列容量  */
    ): port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false)
    {
        srcDir_ = getcwd(nullptr, 256); //getcwd from unistd.h
        assert(srcDir_);
        strncat(srcDir_, "/resources/", 16); // Append characters from string
        HttpConn::userCount = 0;
        HttpConn::srcDir = srcDir_;
        SqlConnPool::Instance()->Init('localhost', sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);

        InitEventMode_(trigMode);
        if(!InitSocket_()) { isClose_ = true;}

        if(openLog) { // 如果 openLog = false 可能在别的地方调用 Log::Instance() 可能会出错
            Log::Instance()->init(logLevel, "./log", ".log", logQueSize);
            if(isClose_){ 
                LOG_ERROR("========== Server init error!==========");
            }
            else{
                LOG_INFO("========== Server init ==========");
                LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
                LOG_INFO("Listen Mode: %s, OpenConn Mode: %s", 
                    (listenEvent_ & EPOLLET ? "ET":"LT"), 
                    (connEvent_ & EPOLLET ? "ET" : "LT"));
                LOG_INFO("LogSys level: %d", logLevel);
                LOG_INFO("srcDir: %s", HttpConn::srcDir);
                LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
            }

        }
    }


WebServer::~WebServer(){

}

void WebServer::InitEventMode_(int trigMode){
    // LT: 水平触发模式
    // ET：边缘触发模式
    /* EPOLLONESHOT：一个线程读取某个socket上的数据后开始处理数据，在处理过程中该socket上又有新数据可读，此时另一个线程被唤醒读取，此时出现两个线程处理同一个socket；我们期望的是一个socket连接在任一时刻都只被一个线程处理，通过epoll_ctl对该文件描述符注册epolloneshot事件，一个线程处理socket时，其他线程将无法处理，当该线程处理完后，需要通过epoll_ctl重置epolloneshot事件 */
    listenEvent_ = EPOLLRDHUP; //
    connEvent_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent_ |= EPOLLET;
        break;
    case 2:
        listenEvent_ |= EPOLLET;
        break;
    case 3:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    default:
        listenEvent_ |= EPOLLET;
        connEvent_ |= EPOLLET;
        break;
    }
    HttpConn::isET = (connEvent_ & EPOLLET);
}

/* Create listenFd */
bool WebServer::InitSocket_() {
    int ret;
    /* 
    5.1.3 专用 socket 地址
        sockaddr_in  专用于 IPV4; sockaddr_in6 专用于 IPV6；
        Note-[INADDR_ANY: This is an IP address that is used when we don't want to bind a socket to any specific IP. 
        Basically, while implementing communication, we need to bind our socket to an IP address. 
        When we don't know the IP address of our machine, we can use the special IP address INADDR_ANY .]
    */
    struct sockaddr_in addr;
    if(port_ > 65535 || port_ < 1024) { // port number is 2 byte in TCP protocol: 2^16 = 65536
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }
    addr.sin_family = AF_INET; // #include <netinet/in.h>; TCP/IPV4 协议族
    // 5.1.1 主机字节序和网咯字节序
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // htonl: host to network long 长整型函数通常用来转换 IP 地址；
    addr.sin_port = htons(port_);
    struct linger optLinger = { 0 }; // 短整型函数用来转换端口号；
    if(openLinger_){
        /* 优雅关闭: 直到所剩数据发送完毕或超时 */
        optLinger.l_onoff = 1;
        optLinger.l_linger = 1;
        /*
            三种断开方式：
            1. l_onoff = 0; l_linger忽略
                close()立刻返回，底层会将未发送完的数据发送完成后再释放资源，即优雅退出。
            2. l_onoff != 0; l_linger = 0;
                close()立刻返回，但不会发送未发送完成的数据，而是通过一个REST包强制的关闭socket描述符，即强制退出。
            3. l_onoff != 0; l_linger > 0;
                close()不会立刻返回，内核会延迟一段时间，这个时间就由l_linger的值来决定。如果超时时间到达之前，发送完未发送的数据(包括FIN包)并得到另一端的确认，close()会返回正确，socket描述符优雅性退出。否则，close()会直接返回错误值，未发送数据丢失，socket描述符被强制性退出。需要注意的时，如果socket描述符被设置为非堵塞型，则close()会直接返回值。
            具体用法：
                struct linger ling = {0, 0};
                setsockopt(socketfd, SOL_SOCKET, SO_LINGER, (void*)&ling, sizeof(ling));
        */
    }
    /*
    5.2 创建 socket
        PF_INET 用于 IPV4, PF_INET6 用于 IPV6; 
        SOCK_STREAM(流服务) 用于 TCP 协议；SOCK_UGRAM(数据报服务) 用于 UDP 协议; 
        成功时返回一个 socket 文件描述符；失败时返回 -1
    */
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd_ < 0){
        LOG_ERROR("Create socket error!", port_);
        return false;
    }
    /*
    setsockopt()函数：设置socket状态。
    int setsockopt(int s, int level, int optname, const void * optval, ,socklen_t optlen);
    函数说明：setsockopt()用来设置参数s 所指定的socket 状态. 参数level 代表欲设置的网络层, 一般设成SOL_SOCKET 以存取socket 层. 参数optname 代表欲设置的选项, 有下列几种数值:
        SO_DEBUG 打开或关闭排错模式
        SO_REUSEADDR 允许在bind ()过程中本地地址可重复使用
        SO_TYPE 返回socket 形态.
        SO_ERROR 返回socket 已发生的错误原因
        SO_DONTROUTE 送出的数据包不要利用路由设备来传输.
        SO_BROADCAST 使用广播方式传送
        SO_SNDBUF 设置送出的暂存区大小
        SO_RCVBUF 设置接收的暂存区大小
        SO_KEEPALIVE 定期确定连线是否已终止.
        SO_OOBINLINE 当接收到OOB 数据时会马上送至标准输入设备
        SO_LINGER 确保数据安全且可靠的传送出去.
    参数 optval 代表欲设置的值, 参数 optlen 则为 optval 的长度.

    */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_LINGER,
        &optLinger, sizeof(optLinger));
    if(ret < 0){
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval = 1;
    /* 端口复用 */
    /* 只有最后一个套接字会正常接收数据。 */
    ret = setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(ret < 0) {
        LOG_ERROR("set socket setsockopt error !");
        close(listenFd_);
        return false;
    }

    // 5.3 命名 socket
    // 成功时返回 0；失败时返回 -1
    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0){
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    // 5.4 监听 socket
    // 成功时返回 0；失败时返回 -1
    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = epoller_->AddFd(listenFd_, listenEvent_ | EPOLLIN);
    if(ret == 0) { // Note: this is ret == 0
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    /*
        https://blog.csdn.net/ccw_922/article/details/124625718
        WebServer为什么需要将socket设置为非阻塞？
        
    */
    SetFdNonblock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}


void WebServer::Start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    if(!isClose_) 
        LOG_INFO("========== Server start ==========");
    while(!isClose_) {
        if(timeoutMS_ > 0) {
            timeMS = timer_->GetNextTick();
        }
        // 9.3.2 epoll_wait 函数
        // 将所有就绪的事件从内核事件表 epollfd 中复制到 events 数组。
        // number 都是索引 epoll 返回的就绪文件描述符
        int eventCnt = epoller_->Wait(timeMS);
        
        for(int i = 0; i < eventCnt; i++) { // 仅遍历就绪的 number 个文件描述符
            /* 处理事件 */
            int fd = epoller_->GetEventFd(i); // sockfd 肯定就绪，直接处理
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listenFd_) { //处理新到的客户连接
                DealListen_();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
/*
    EPOLLRDHUP: Stream socket peer closed connection, or shut down writing
              half of connection.
    EPOLLHUP: Hang up happened on the associated file descriptor.
    EPOLLERR: Error condition happened on the associated file
              descriptor. 
*/
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd])
            }
            else if(events & EPOLLIN){
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }
        }
    }
}


int WebServer::SetFdNonblock(int fd) {
    assert(fd > 0);
    /*  fcntl - manipulate file descriptor */
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}

void WebServer::SendError_(int fd, const char*info) {
    assert(fd > 0);
    int ret = send(fd, info, strlen(info), 0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].init(fd, addr); // HttpConn->init
    if(timeoutMS_ > 0) {
        timer_->add(fd, timeoutMS_, std::bind(&WebServer::CloseConn_, this, &users_[fd]));
    }
    epoller_->AddFd(fd, EPOLLIN | connEvent_);
    SetFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}


void WebServer::DealListen_() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    do{
        // 5.5 接受连接
        int fd = accept(listenFd_, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(HttpConn::userCount >= MAX_FD){
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd, addr);
    } while (listenEvent_ & EPOLLET);
}

void WebServer::DealRead_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnRead_, this, client));
}

void WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&WebServer::OnWrite_, this, client)); // std::bind 解释 https://www.cnblogs.com/xusd-null/p/3698969.html

}

void WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
}

void WebServer::OnRead_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    }else{
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}


void WebServer::OnWrite_(HttpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->ToWriteBytes() == 0) {
        /* 传输完成 */
        if(client->IsKeepAlive()) {
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}