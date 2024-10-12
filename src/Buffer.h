#pragma once

#include <vector>
#include <string>
#include <algorithm>

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t initialSize = kInitialSize)
        :buffer_(kCheapPrepend + kInitialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
    {}

    size_t readableBytes() const
    {
        return this->writerIndex_ - this->readerIndex_;
    }

    size_t writableBytes() const
    {
        return this->buffer_.size() - this->writerIndex_;
    }

    size_t prependableBytes() const
    {
        return this->readerIndex_;
    }

    // 返回缓冲区中可读数据的起始地址
    const char* peek() const
    {
        return begin()+readerIndex_;
    }

    void retrieve(size_t len)
    {
        if(len < readableBytes()){
            this->readerIndex_ += len;      // 读取了可读缓冲区的一部分
        }else{
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        this->readerIndex_ = this->writerIndex_ = this->kCheapPrepend;
    }

    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }

    void ensureWritableBytes(size_t len)
    {
        if(writableBytes() < len){
            makeSpace(len);   
        }
    }

    void append(const char* data,size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data,data+len,beginWrite());
        this->writerIndex_ += len;
    }

    char* beginWrite()
    {
        return begin() + this->writerIndex_;
    }
    const char* beginWrite() const
    {
        return begin() + this->writerIndex_;
    }


    ssize_t readFd(int fd,int* saveError);

    ssize_t writeFd(int fd,int* saveError);

private:
    void makeSpace(size_t len)
    {
        if(writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            this->buffer_.resize(this->writerIndex_ + len);
        }else{
            size_t readable = readableBytes();
            std::copy(begin() + this->readerIndex_,begin() + this->writerIndex_,begin() + kCheapPrepend);
            this->readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readable;
        }
    }

    char* begin()
    {
        return &*this->buffer_.begin();
    }

    const char* begin() const
    {
        return &*this->buffer_.begin();
    }

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};