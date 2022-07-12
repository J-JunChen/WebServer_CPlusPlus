#ifndef HTTP_CONN_H
#define HTTP_CONN_H


#include "httpresponse.h"
#include "httprequest.h"

#include "../log/log.h"
#include "../buffer/buffer.h"

class HttpConn
{
private:
    int fd_;
    struct sockaddr_in addr_;
    bool isClose_;

    HttpResponse response_;

    Buffer readBuff_; // 读缓冲区
    Buffer writeBuff_; // 写缓冲区


public:
    HttpConn(/* args */);
    ~HttpConn();

    void init(int sockFd, const sockaddr_in& addr);

    void Close();

    int GetFd() const;
    const char* GetIP() const;
    int GetPort() const;
    sockaddr_in GetAddr() const;

    bool process();

    static bool isET;
    static const char* srcDir;
    /*
        std::atomic：Atomically increments or decrements the current value. The operation is read-modify-write operation.
    */
    static std::atmoic<int> userCount; 

};

#endif