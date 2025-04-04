#pragma once

#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

namespace XH {

#ifdef __cpp_lib_jthread
using thread_t = std::jthread;
#else
using thread_t = std::thread;
#endif

enum class os_thread_priority
{
#if defined(__WIN32__)
    idle = THREAD_PRIORITY_IDLE,
    low = THREAD_PRIORITY_LOWEST,
    normal = THREAD_PRIORITY_NORMAL,
    above_normal = THREAD_PRIORITY_ABOVE_NORMAL,
    high = THREAD_PRIORITY_HIGHEST,
    realtime = THREAD_PRIORITY_TIME_CRITICAL,
#elif defined(__linux__) || defined(__APPLE__)
    idle = 0,
    low = 1,
    normal = 2,
    above_normal = 3,
    high = 4,
    realtime = 5,
#endif
};

class [[nodiscard]] this_thread
{
public:
    [[nodiscard]] static std::optional<std::size_t> get_index() noexcept
    {
        return m_index;
    }

    [[nodiscard]] static std::optional<void*> get_pool() noexcept
    {
        return m_pool;
    }

    [[nodiscard]] static std::optional<std::string> get_os_thread_name() noexcept;
    static bool set_os_thread_name(const std::string& name) noexcept;

    

#ifdef XH_THREAD_POOL_NATIVE_EXTENTIONS
    [[nodiscard]] static std::optional<std : vector<bool>> get_os_thread_affinity() noexcept;
    static bool set_os_thread_affinity(std::vector<bool> affinity) noexcept;

    [[nodiscard]] static std::optional<os_thread_priority> get_os_thread_priority() noexcept;
    static bool set_os_thread_priority(const os_thread_priority priority) noexcept;
#endif

private:
    static thread_local std::optional<std::size_t> m_index;
    static thread_local std::optional<void*> m_pool;
};

} // namespace XH