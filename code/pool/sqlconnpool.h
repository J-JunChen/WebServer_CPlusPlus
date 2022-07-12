#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <assert.h>
#include <mysql/mysql.h>
#include <semaphore.h>

#include "../log/log.h"

class SqlConnPool
{
private:
    SqlConnPool(/* args */);
    ~SqlConnPool();

    int MAX_CONN_;

    std:queue<MYSQL *> connQue_;
    std::mutex mtx_; // 互斥量
    sem_t semId_; // 信号量

public:
    static SqlConnPool *Instance();

    void Init(const char* host, int port,
            const char* user, const char* pwd,
            const* dbName, int connSize);
    
};

SqlConnPool::SqlConnPool(/* args */)
{
}

SqlConnPool::~SqlConnPool()
{
}



#endif