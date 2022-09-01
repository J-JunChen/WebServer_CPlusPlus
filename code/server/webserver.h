#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unistd.h> // close()
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>       // fcntl()

#include "epoller.h"
#include "../http/httpconn.h"
#include "../pool/sqlconnpool.h"
#include "../timer/heaptimer.h"
#include "../pool/threadpool.h"

class WebServer
{
private:
    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    char* srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;

    static const int MAX_FD = 65536;  // 2^16 = 65536


private:
    bool InitSocket_(); 
    void InitEventMode_(int trigMode);
    static int SetFdNonblock(int fd);

    void AddClient_(int fd, sockaddr_in addr);
    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);

    void SendError_(int fd, const char*info);
    void CloseConn_(HttpConn* client);
    void ExtentTime_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);

public:

    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, /* 端口，ET模式，timeoutMs，优雅退出 */
        int sqlPort, const char* sqlUser, const char* sqlPwd, const char* dbName, /* Mysql配置 */
        int connPoolNum, int threadNum, /* 连接池数量，线程池数量 */
        bool openLog, int logLevel, int logQueSize /* 日志开关，日志等级，日志异步队列容量  */
    );
    
    ~WebServer();
    void Start();
};


#endif //WEBSERVER_H