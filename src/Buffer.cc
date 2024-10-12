#include "Buffer.h"

#include <sys/uio.h>
#include <errno.h>
#include <unistd.h>

ssize_t Buffer::readFd(int fd,int* saveError){
    char extrabuf[65536] = {0};
    struct iovec vec[2];
    
    const size_t writable = writableBytes();    // 缓冲区剩余可写空间大小
    
    vec[0].iov_base = begin()+ writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);

    const int iovcnt = (writable < sizeof(extrabuf)) ? 2 : 1;
    const ssize_t n = ::readv(fd,vec,iovcnt);

    if(n < 0){
        *saveError = errno;   
    }else if(n <= writable){
        this->writerIndex_ += n;
    }else{
        this->writerIndex_ = this->buffer_.size();
        append(extrabuf,n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd,int* saveError)
{
    ssize_t n = ::write(fd,peek(),readableBytes());
    if(n < 0){
        *saveError = errno;   
    }

    return n;   

}