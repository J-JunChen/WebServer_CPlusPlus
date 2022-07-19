#ifndef LOG_H
#define LOG_H

#include <thread>

#include "blockqueue.h"
#include "../buffer/buffer.h"

class Log
{
private:
    static const int LOG_NAME_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char* path_;
    const char* suffix_;

    int MAX_LINES_;
    int lineCount_;
    bool isAsync_;

    bool isOpen_;
    int level_;
    int toDay_;

    Buffer buff_;


    FILE* fp_;
    std::unique_ptr<BlockDeque<std::string>> deque_; 
    std::unique_ptr<std::thread> writeThread_;
    std::mutex mtx_; // The mutex class is a synchronization primitive that can be used to protect shared data from being simultaneously accessed by multiple threads.

private:
    Log();
    virtual ~Log(); //为什么要用到虚的析构函数？Log() 有被继承吗？
    void AsyncWrite_();
    void AppendLogLevelTitle_(int level);
    
public:

    void init(int level, const char* pth = "./log",
                const char* suffix=".log",
                int maxQueueCapacity = 1024);
    static Log* Instance();
    static void FlushLogThread();
    
    void write(int level, const char *format,...);
    void flush();

    int GetLevel();
    void SetLevel(int level);
    bool IsOpen() { return isOpen_; }


};



// define的多行定义:do{...}while(0)，关键是要在每一个换行的时候加上一个"\"
// ... 可变参数
#define LOG_BASE(level, format, ...)\
    do{\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) LOG_BASE(0, format,  ##__VA_ARGS__);
#define LOG_INFO(format, ...) LOG_BASE(1, format,  ##__VA_ARGS__);
#define LOG_WARN(format, ...) LOG_BASE(2, format,  ##__VA_ARGS__);
#define LOG_ERROR(format, ...) LOG_BASE(3, format,  ##__VA_ARGS__);

 
#endif