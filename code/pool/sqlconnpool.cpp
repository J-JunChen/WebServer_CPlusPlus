#include "sqlconnpool.h"
using namespace std;

static SqlConnPool *Instance(){
    static SqlConnPool connPool; // local static variable
    return &connPool; 
};

void SqlConnPool::Init(const char* host, int port,
        const char* user, const char* pwd,
        const* dbName, int connSize = 10){
    assert(connSize > 0);
    for (int i = 0; i< connSize; i++){
        MYSQL *sql = nullptr;
        sql = mysql_init(sql);
        if(!sql){
            LOG_ERROR("Mysql init error!");
            assert(sql);
        }
        sql = mysql_real_connect(sql, host,
            user, pwd, dbName, port, nullptr, 0); //mysql_connect() 连接一个MySQL服务器。该函数 deprecated；使用mysql_real_connect()代替。
        if (!sql){
            LOG_ERROR("Mysql Connect error!")
        }
        connQue_.push(sql);
    }
    MAX_CONN_ = connSize;
    /* int sem_init(sem_t *sem, int pshared, unsigned int value);
        The pshared argument indicates whether this semaphore is to be shared between the threads of a process, or between processes. 
        If pshared has the value 0, then the semaphore is shared between the threads of a process; If pshared is nonzero, then the semaphore is shared between
       processes.
        The value argument specifies the initial value for the semaphore.
    */
    sem_init(&semId_, 0, MAX_CONN_);
}