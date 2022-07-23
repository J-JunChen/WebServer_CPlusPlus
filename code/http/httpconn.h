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

    /*
        当用于类声明时，即静态成员变量和静态成员函数，static表示所有类对象共享这些数据和函数，而非每个类对象独有

        static变量在类的声明中不占用内存，必须在.cpp文件中定义类静态变量以分配内存
    */

};

#endif