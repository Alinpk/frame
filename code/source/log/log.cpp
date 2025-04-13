#include "basic/log.h"
#include <future>
#include <unistd.h>

Log &Log::GetInstance()
{
    static Log instance;
    return instance;
}

bool Log::Init(std::string_view file, uint32_t queueSize, int levelMask)
{
    std::filesystem::path p(file);

    if (!std::filesystem::exists(p.parent_path()) && !std::filesystem::create_directories(p.parent_path())) {
        return false;
    }

    // format of full path : [path + logname + Y-M-D], 这里是为了后续的统一处理
    m_fullPath = p;
    m_filename = p.filename();
    m_levelMask = levelMask;
    m_queueSize = queueSize;

    CreateLog(fmt::format("{:%Y-%m-%d:%H:%M}", std::chrono::system_clock::now()));
    return true;
}

void Log::Flush()
{
    // 为了避免有两个线程在做文件持久化
    std::unique_lock lk(mut, std::defer_lock);
    if (!lk.try_lock()) {
        return;
    }

    std::optional<std::string> log;
    while ((log = m_logQueue.Pop())) {
        if (std::string t = log.value().substr(OFF_SET, TAIL_LEN); m_logtime != t || m_fp == nullptr) {
            CreateLog(t);
        }
        // 偷懒没判断返回
        // 且这里并没有做磁盘写入的操作
        std::fwrite(log.value().data(), sizeof(log.value().front()), log.value().size(), m_fp);
        fsync(fileno(m_fp));
    }
}

void Log::AutoFlush()
{
    if (m_queueSize < m_logQueue.Size()) {
        (void)std::async(std::launch::async, [this]() { Flush(); });
    }
}

void Log::CreateLog(std::string time)
{
    Stop();

    m_logtime = time.substr(0, TAIL_LEN);  // xxxx-xx-xx

    m_fullPath = m_fullPath.parent_path().append(m_filename + "_" + m_logtime);

    m_fp = fopen(m_fullPath.c_str(), "a");
}

void Log::Stop()
{
    Flush();
    if (m_fp != nullptr) {
        std::fclose(m_fp);
        m_fp = nullptr;
    }
}

Log::Log()
{
    m_fp = nullptr;
}

Log::~Log()
{
    Flush();
}
