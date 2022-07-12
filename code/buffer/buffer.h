#ifndef BUFFER_H
#define BUFFER_H

class Buffer
{
private:
    std::vector<char> buffer_;

public:
    Buffer(int initBuffSize = 1024);
    ~Buffer() = default;

    void RetrieveAll();
    size_t ReadableBytes() const ;

};



#endif