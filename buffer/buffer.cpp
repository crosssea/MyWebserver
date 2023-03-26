#include "buffer.h"

Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0) {};

size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

size_t Buffer::PrependableBytes() const {
    return readPos_;
}

const char* Buffer::Peek() const {
    return BeginPtr_() + readPos_;
}

void Buffer::Retrieve(size_t len) {
    assert(len <= ReadableBytes());
    readPos_ += len;
}

void Buffer::RetrieveUntil(const char* end) {
   assert(Peek() <= end);
   Retrieve(end - Peek()); 
}

void Buffer::RetrieveAll() {
    bzero(buffer_.data(), buffer_.size());
    readPos_ = 0;
    writePos_ = 0;
}
std::string Buffer::RetrieveAllToStr() {
    std::string res(Peek(), ReadableBytes());
    RetrieveAll();
    return res;
}

const char* Buffer::BeginWriteConst() const {
    return BeginPtr_() + writePos_;
}

char* Buffer::BeginWrite() {
    return BeginPtr_() + writePos_;
}

void Buffer::HasWritten(size_t len){
    writePos_ += len;
}

void Buffer::Append(const std::string &str) {
    Append(str.data(), str.length());
}

/*
append len chars to buffer
*/
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWritable(len);
    std::copy(str, str+len, BeginWrite());
    HasWritten(len);
}

void Buffer::Append(const void* data, size_t len) {
    assert(data);
    Append(static_cast<const char*>(data), len); // static cast cannot remove const 
}

void Buffer::Append(const Buffer &buffer) {
    Append(buffer.Peek(), buffer.ReadableBytes());
}

void Buffer::EnsureWritable(size_t len) {
    if (WritableBytes() < len) {
        MakeSpace_(len);
    }
    assert(WritableBytes() >= len);
}

/*
使用两个iovec， 调用readv从fd中读取数据
如果第一个满了，则会继续往下进行存放
而如果缓冲区不足以存放，则缓冲区会进行扩容
*/

ssize_t Buffer::ReadFd(int fd, int *Errno) {
    char buff[65535];
    struct iovec iov[2];
    size_t writable = WritableBytes();
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    const ssize_t len = readv(fd, iov, 2);
    if (len < 0) {
        *Errno = errno;
    } else if (static_cast<size_t>(len) <= writable) {
        writePos_ += len;
    } else {
        writePos_ = buffer_.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int *Errno) {
    size_t readSize = ReadableBytes();
    size_t len = write(fd, Peek(), readSize);
    if (len < 0) {
        *Errno = errno;
        return len;
    }
    readPos_ += len;
    return len;
}

char* Buffer::BeginPtr_ () {
    return buffer_.data();
}

const char* Buffer::BeginPtr_ () const{
    return buffer_.data();
}

void Buffer::MakeSpace_ (size_t len) {
    if (WritableBytes() + PrependableBytes() < len) {
        buffer_.resize(writePos_ + len + 1); // 可以考虑更改一下扩容规则
    } else {
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginWrite(), BeginPtr_()); // 不可以一个带const一个不带
        readPos_ = 0;
        writePos_ = readable;
        assert(readable == ReadableBytes());
    }
}