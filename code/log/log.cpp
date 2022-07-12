#include "log.h"

using namespace std;

Log::Log()
{
    lineCount_ = 0;
    isAsync_ = false; // 所谓同步日志，即当输出日志时，必须等待日志输出语句执行完毕后，才能执行后面的业务逻辑语句
    writeThread_ = nullptr;
    deque_ = nullptr;
    toDay_ = 0;     // toDay_ = sysTime.tm_mday; (tm_mday: day of the month)
    fp_ = nullptr; // fp_ = fopen(fileName, "a");
}

Log::~Log()
{
}

void Log::init(int level=1, const char* path,
                const char* suffix, int maxQueueCapacity){
    isOpen_ = true;
    level_ = level;
    if(maxQueueSize > 0) {
        isAsync_ = true; //使用异步日志进行输出时，日志输出语句与业务逻辑语句并不是在同一个线程中运行，而是有专门的线程用于进行日志输出操作，处理业务逻辑的主线程不用等待即可执行后续业务逻辑。
        if(!deque_){
            unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque_ = move(newDeque);

            unique_ptr<std::thread> newThread(new std::thread(FlushLogThread));
            writeThread_ = move(NewThread);
        }
    }else{
        isAsync_ = false;
    }

    lineCount_ = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    path_ = path;
    suffix_ = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    /*
        snprintf()函数用于将格式化的数据写入字符串，其原型为：
    int snprintf(char *str, int n, char * format [, argument, ...]);

        【参数】str为要写入的字符串；n为要写入的字符的最大数目，超过n会被截断；format为格式化字符串，与printf()函数相同；argument为变量。

        【返回值】成功则返回参数str 字符串长度，失败则返回-1，错误原因存于errno 中。
    */
    snprintf(fileName, LOG_NAME_LEN-1, "%s/%04d_%02d_%02d%s",
            path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
    toDay_ = t.tm_mday;
    
    {
        /*
            lock_guard实现原理RAII：资源获取即初始化
            lock_guard构造函数执行了mutex::lock()
            lock_guard析构函数执行了mutex::unlock()
        */
        lock_guard<mutex> locker(mtx_);
        buff_.RetrieveAll();
        if(fp_){
            flush();
            fclose(fp_); 
        }

        fp_ = fopen(fileName, "a") // a: append
        if(fp_ == nullptr) {
            mkdir(path_, 0777);
            fp_ = fopen(fileName, "a");
        }
        assert(fp_ != nullptr);
    }
}

Log* Log::Instance() {
    static Log inst;
    return &inst;
}

void Log::AsyncWrite_(){
    string str = "";
    while(deque_->pop(str)){
        /*
            lock_guard实现原理RAII：资源获取即初始化
            lock_guard构造函数执行了mutex::lock()
            lock_guard析构函数执行了mutex::unlock()
        */
        lock_guard<mutex> locker(mtx_);
        // 相当于在这里加锁 mutex mtx_.lock();
        /*
            c_str()函数返回一个指向正规C字符串的指针常量, 内容与本string串相同。
            这是为了与c语言兼容，在c语言中没有string类型，故必须通过string类对象的成员函数c_str()把string 对象转换成c中的字符串样式。
        */
        fputs(str.c_str, fp_);
        // 相当于在这里解锁 mutex mtx_.unlock();
    }
}

void Log::flush() {
    if(isAsync_) { 
        deque_->flush(); 
    }
    /*
        C 库函数 int fflush(FILE *stream) 刷新流 stream 的输出缓冲区，即清空文件缓冲区。
    */
    fflush(fp_);
}

void Log::FlushLogThread(){
    Log::Instance()->AsyncWrite_();
}