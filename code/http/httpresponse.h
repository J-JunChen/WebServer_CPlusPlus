#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <sys/mman.h>    // mmap, munmap


class HttpResponse
{
private:
    char* mmFile_;


public:
    HttpResponse(/* args */);
    ~HttpResponse();

    void UnmapFile();

};


#endif //HTTP_RESPONSE_H
