#include "httpresponse.h"


HttpResponse::HttpResponse(/* args */){
}

HttpResponse::~HttpResponse(){
}


void HttpResponse::UnmapFile() {
    if(mmFile_) {
        /*
定义函数：int munmap(void *start, size_t length);

函数说明：munmap()用来取消参数start 所指的映射内存起始地址，参数length 则是欲取消的内存大小。当进程结束或利用exec 相关函数来执行其他程序时，映射内存会自动解除，但关闭对应的文件描述词时不会解除映射。

返回值：如果解除映射成功则返回0，否则返回－1。错误原因存于errno 中错误代码EINVAL参数 start 或length 不合法。
        */
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}