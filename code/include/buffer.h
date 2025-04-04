#ifndef BUFFER_H
#define BUFFER_H

#include <atomic>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>

class Buffer
{
public:
    Buffer(uint32_t initBuffSize = 1024);
    ~Buffer();

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t ReadBytes() const;

    std::string_view Peek() const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveAll();
    std::string RetrieveAllToStr();

    std::string_view BeginWriteConst() const;
    char* BeginWrite();

    void Append(std::string_view str);
    void Append(const Buffer& buff);

    size_t ReadFd(int fd, int* Errno);
    size_t WriteFd(int fd, int* Errno);

private:
    const char* BeginPtr() const;
    void MakeSpace(size_t len);

private:
    std::vector<char> m_buff;
    std::atomic<std::size_t> m_readPos;
    std::atomic<std::size_t> m_writePos;
};

#endif