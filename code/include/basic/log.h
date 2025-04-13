#ifndef LOG_H
#define LOG_H
#include <stdint.h>
#include <cstdio>
#include <string>
#include <string_view>
#include <filesystem>
#include "fmt/format-inl.h"
#include "fmt/chrono.h"
#include "thread_safe_queue.h"

#define LOG_DEBUG(fmt, ...)                                 \
    {                                                       \
        Log::GetInstance().WriteLog(0, fmt, ##__VA_ARGS__); \
    }
#define LOG_INFO(fmt, ...)                                  \
    {                                                       \
        Log::GetInstance().WriteLog(1, fmt, ##__VA_ARGS__); \
    }
#define LOG_WARN(fmt, ...)                                  \
    {                                                       \
        Log::GetInstance().WriteLog(2, fmt, ##__VA_ARGS__); \
    }
#define LOG_ERROR(fmt, ...)                                 \
    {                                                       \
        Log::GetInstance().WriteLog(3, fmt, ##__VA_ARGS__); \
    }

constexpr char LOG_LEVEL[4] = {'D', 'I', 'W', 'E'};

class Log {
public:
    // ensure Init func has been called
    static Log &GetInstance();
    bool Init(std::string_view fullpath, uint32_t queueSize, int levelMask);

    template <typename... Args>
    void WriteLog(int level, std::string fmt, Args &&...args);

    void Flush();

    void Stop();

private:
    // need change to new file or buffer reach m_logQueue
    void AutoFlush();

    void CreateLog(std::string time);

    Log();
    virtual ~Log();

private:
    std::string m_filename;
    std::string m_logtime;
    std::filesystem::path m_fullPath;
    int m_levelMask;
    uint32_t m_queueSize;
    ThreadSafeQueue<std::string> m_logQueue;
    std::FILE* m_fp;
    std::mutex mut;

    static constexpr size_t TAIL_LEN = 10;
    static constexpr size_t OFF_SET = 6;
};

template <typename... Args>
void Log::WriteLog(int level, std::string fmt, Args &&...args)
{
    if (level < m_levelMask) {
        return;
    }
    auto localTimeStr = fmt::format("{:%Y-%m-%d:%H:%M}", std::chrono::system_clock::now());
    // [level] | time | message
    fmt = "[{}] | {} |" + fmt + "\n";
    GetInstance().m_logQueue.Push(fmt::format(fmt::runtime(fmt), LOG_LEVEL[level], localTimeStr, args...));
}
#endif