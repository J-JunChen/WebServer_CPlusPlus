#include "buffer.h"



Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {}

Buffer::~Buffer()
{
}

void Buffer::RetrieveAll() {
    /*
        The bzero() function erases the data in the n bytes of the memory starting at the location pointed to by s, by writing zeros (bytes containing '\0') to that area.
    */
    bzero(&buffer_[0], buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}