#include "basic/thread.h"


namespace XH {

std::optional<std::string> this_thread::get_os_thread_name() noexcept
{
#if defined(__WIN32__)
    // Windows-specific code to get thread name
    char name[256];
    DWORD length = GetThreadDescription(GetCurrentThread(), name, sizeof(name));
    if (length == 0)
        return std::nullopt;
    return std::string(name, length);
#elif defined(__linux__) || defined(__APPLE__)
    // Linux and macOS-specific code to get thread name
    #ifdef __APPLE__
        constexpr std:size_t buffer_size = 16;
    #else
        constexpr std::size_t buffer_size = 64;
    #endif
    char name[buffer_size] = {'\0'};
    if (pthread_getname_np(pthread_self(), name, sizeof(name)) != 0)
        return std::nullopt;
    return std::string(name);
#else
    return std::nullopt; // Not supported on this platform
#endif
}

bool this_thread::set_os_thread_name(const std::string& name) noexcept
{
#if defined(_WIN32)
    // On Windows thread names are wide strings, so we need to convert them from normal strings.
    const int size = MultiByteToWideChar(CP_UTF8, 0, name.data(), -1, nullptr, 0);
    if (size == 0)
        return false;
    std::wstring wide(static_cast<std::size_t>(size), 0);
    if (MultiByteToWideChar(CP_UTF8, 0, name.data(), -1, wide.data(), size) == 0)
        return false;
    const HRESULT hr = SetThreadDescription(GetCurrentThread(), wide.data());
    return SUCCEEDED(hr);
#elif defined(__linux__)
    // On Linux this is straightforward.
    return pthread_setname_np(pthread_self(), name.data()) == 0;
#elif defined(__APPLE__)
    // On macOS, unlike Linux, a thread can only set a name for itself, so the signature is different.
    return pthread_setname_np(name.data()) == 0;
#endif
}

#ifdef XH_THREAD_POOL_NATIVE_EXTENTIONS
std::optional<std:vector<bool>> this_thread::get_os_thread_affinity() noexcept
{
    #if defined(__WIN32__)
        // Windows-specific code to get thread affinity
        DWORD_PTR mask = GetProcessAffinityMask(GetCurrentProcess(), nullptr, nullptr);
        std::vector<bool> affinity(std::thread::hardware_concurrency());
        for (std::size_t i = 0; i < affinity.size(); ++i)
        {
            affinity[i] = (mask & (1 << i)) != 0;
        }
        return affinity;
    #elif defined(__linux__)
        // Linux-specific code to get thread affinity
        cpu_set_t mask;
        CPU_ZERO(&mask);
        if (sched_getaffinity(0, sizeof(mask), &mask) == -1)
            return std::nullopt;

        std::vector<bool> affinity(std::thread::hardware_concurrency());
        for (std::size_t i = 0; i < affinity.size(); ++i)
        {
            affinity[i] = CPU_ISSET(i, &mask);
        }
        return affinity;
    #else
        return std::nullopt;
    #endif
}

bool this_thread::set_os_thread_affinity(std::vector<bool> affinity) noexcept
{
#if defined(__WIN32__)
    // Windows-specific code to set thread affinity
    DWORD_PTR mask = 0;
    for (std::size_t i = 0; i < affinity.size(); ++i)
    {
        if (affinity[i])
            mask |= (1 << i);
    }
    return SetThreadAffinityMask(GetCurrentThread(), mask) != 0;
#elif defined(__linux__)
    // Linux-specific code to set thread affinity
    cpu_set_t mask;
    CPU_ZERO(&mask);
    for (std::size_t i = 0; i < affinity.size(); ++i)
    {
        if (affinity[i])
            CPU_SET(i, &mask);
    }
    return sched_setaffinity(0, sizeof(mask), &mask) == 0;
#else
    return false; // Not supported on this platform
#endif
}

std::optional<os_thread_priority> this_thread::get_os_thread_priority() noexcept
{
#if defined(_WIN32)
    // On Windows, this is straightforward.
    const int priority = GetThreadPriority(GetCurrentThread());
    if (priority == THREAD_PRIORITY_ERROR_RETURN)
        return std::nullopt;
    return static_cast<os_thread_priority>(priority);
#elif defined(__linux__)
    // On Linux, we distill the choices of scheduling policy, priority, and "nice" value into 7 pre-defined levels, for simplicity and portability. The total number of possible combinations of policies and priorities is much larger, so if the value was set via any means other than `BS::this_thread::set_os_thread_priority()`, it may not match one of our pre-defined values.
    int policy = 0;
    struct sched_param param = {};
    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0)
        return std::nullopt;
    if (policy == SCHED_FIFO && param.sched_priority == sched_get_priority_max(SCHED_FIFO))
    {
        // The only pre-defined priority that uses SCHED_FIFO and the maximum available priority value is the "realtime" priority.
        return os_thread_priority::realtime;
    }
    if (policy == SCHED_RR && param.sched_priority == sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) / 2)
    {
        // The only pre-defined priority that uses SCHED_RR and a priority in the middle of the available range is the "highest" priority.
        return os_thread_priority::highest;
    }
    #ifdef __linux__
    if (policy == SCHED_IDLE)
    {
        // The only pre-defined priority that uses SCHED_IDLE is the "idle" priority. Note that this scheduling policy is not available on macOS.
        return os_thread_priority::idle;
    }
    #endif
    if (policy == SCHED_OTHER)
    {
        // For SCHED_OTHER, the result depends on the "nice" value. The usual range is -20 to 19 or 20, with higher values corresponding to lower priorities. Note that `getpriority()` returns -1 on error, but since this does not correspond to any of our pre-defined values, this function will return `std::nullopt` anyway.
        const int nice_val = getpriority(PRIO_PROCESS, static_cast<id_t>(syscall(SYS_gettid)));
        switch (nice_val)
        {
        case PRIO_MIN + 2:
            return os_thread_priority::above_normal;
        case 0:
            return os_thread_priority::normal;
        case (PRIO_MAX / 2) + (PRIO_MAX % 2):
            return os_thread_priority::below_normal;
        case PRIO_MAX - 3:
            return os_thread_priority::lowest;
    #ifdef __APPLE__
        // `SCHED_IDLE` doesn't exist on macOS, so we use the policy `SCHED_OTHER` with a "nice" value of `PRIO_MAX - 2`.
        case PRIO_MAX - 2:
            return os_thread_priority::idle;
    #endif
        default:
            return std::nullopt;
        }
    }
    return std::nullopt;
#elif defined(__APPLE__)
    // On macOS, we distill the choices of scheduling policy and priority into 7 pre-defined levels, for simplicity and portability. The total number of possible combinations of policies and priorities is much larger, so if the value was set via any means other than `BS::this_thread::set_os_thread_priority()`, it may not match one of our pre-defined values.
    int policy = 0;
    struct sched_param param = {};
    if (pthread_getschedparam(pthread_self(), &policy, &param) != 0)
        return std::nullopt;
    if (policy == SCHED_FIFO && param.sched_priority == sched_get_priority_max(SCHED_FIFO))
    {
        // The only pre-defined priority that uses SCHED_FIFO and the maximum available priority value is the "realtime" priority.
        return os_thread_priority::realtime;
    }
    if (policy == SCHED_RR && param.sched_priority == sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) / 2)
    {
        // The only pre-defined priority that uses SCHED_RR and a priority in the middle of the available range is the "highest" priority.
        return os_thread_priority::highest;
    }
    if (policy == SCHED_OTHER)
    {
        // For SCHED_OTHER, the result depends on the specific value of the priority.
        if (param.sched_priority == sched_get_priority_max(SCHED_OTHER))
            return os_thread_priority::above_normal;
        if (param.sched_priority == sched_get_priority_min(SCHED_OTHER) + (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) / 2)
            return os_thread_priority::normal;
        if (param.sched_priority == sched_get_priority_min(SCHED_OTHER) + (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) * 2 / 3)
            return os_thread_priority::below_normal;
        if (param.sched_priority == sched_get_priority_min(SCHED_OTHER) + (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) / 3)
            return os_thread_priority::lowest;
        if (param.sched_priority == sched_get_priority_min(SCHED_OTHER))
            return os_thread_priority::idle;
        return std::nullopt;
    }
    return std::nullopt;
#endif
}

bool this_thread::set_os_thread_priority(const os_thread_priority priority) noexcept
{
#if defined(_WIN32)
    // On Windows, this is straightforward.
    return SetThreadPriority(GetCurrentThread(), static_cast<int>(priority)) != 0;
#elif defined(__linux__)
    // On Linux, we distill the choices of scheduling policy, priority, and "nice" value into 7 pre-defined levels, for simplicity and portability. The total number of possible combinations of policies and priorities is much larger, but allowing more fine-grained control would not be portable.
    int policy = 0;
    struct sched_param param = {};
    std::optional<int> nice_val = std::nullopt;
    switch (priority)
    {
    case os_thread_priority::realtime:
        // "Realtime" pre-defined priority: We use the policy `SCHED_FIFO` with the highest possible priority.
        policy = SCHED_FIFO;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        break;
    case os_thread_priority::highest:
        // "Highest" pre-defined priority: We use the policy `SCHED_RR` ("round-robin") with a priority in the middle of the available range.
        policy = SCHED_RR;
        param.sched_priority = sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) / 2;
        break;
    case os_thread_priority::above_normal:
        // "Above normal" pre-defined priority: We use the policy `SCHED_OTHER` (the default). This policy does not accept a priority value, so priority must be 0. However, we set the "nice" value to the minimum value as given by `PRIO_MIN`, plus 2 (which should evaluate to -18). The usual range is -20 to 19 or 20, with higher values corresponding to lower priorities.
        policy = SCHED_OTHER;
        param.sched_priority = 0;
        nice_val = PRIO_MIN + 2;
        break;
    case os_thread_priority::normal:
        // "Normal" pre-defined priority: We use the policy `SCHED_OTHER`, priority must be 0, and we set the "nice" value to 0 (the default).
        policy = SCHED_OTHER;
        param.sched_priority = 0;
        nice_val = 0;
        break;
    case os_thread_priority::below_normal:
        // "Below normal" pre-defined priority: We use the policy `SCHED_OTHER`, priority must be 0, and we set the "nice" value to half the maximum value as given by `PRIO_MAX`, rounded up (which should evaluate to 10).
        policy = SCHED_OTHER;
        param.sched_priority = 0;
        nice_val = (PRIO_MAX / 2) + (PRIO_MAX % 2);
        break;
    case os_thread_priority::lowest:
        // "Lowest" pre-defined priority: We use the policy `SCHED_OTHER`, priority must be 0, and we set the "nice" value to the maximum value as given by `PRIO_MAX`, minus 3 (which should evaluate to 17).
        policy = SCHED_OTHER;
        param.sched_priority = 0;
        nice_val = PRIO_MAX - 3;
        break;
    case os_thread_priority::idle:
        // "Idle" pre-defined priority on Linux: We use the policy `SCHED_IDLE`, priority must be 0, and we don't touch the "nice" value.
        policy = SCHED_IDLE;
        param.sched_priority = 0;
        break;
    default:
        return false;
    }
    bool success = (pthread_setschedparam(pthread_self(), policy, &param) == 0);
    if (nice_val.has_value())
        success = success && (setpriority(PRIO_PROCESS, static_cast<id_t>(syscall(SYS_gettid)), nice_val.value()) == 0);
    return success;
#elif defined(__APPLE__)
    // On macOS, unlike Linux, the "nice" value is per-process, not per-thread (in compliance with the POSIX standard). However, unlike Linux, `SCHED_OTHER` on macOS does have a range of priorities. So for `realtime` and `highest` priorities we use `SCHED_FIFO` and `SCHED_RR` respectively as for Linux, but for the other priorities we use `SCHED_OTHER` with a priority in the range given by `sched_get_priority_min(SCHED_OTHER)` to `sched_get_priority_max(SCHED_OTHER)`.
    int policy = 0;
    struct sched_param param = {};
    switch (priority)
    {
    case os_thread_priority::realtime:
        // "Realtime" pre-defined priority: We use the policy `SCHED_FIFO` with the highest possible priority.
        policy = SCHED_FIFO;
        param.sched_priority = sched_get_priority_max(SCHED_FIFO);
        break;
    case os_thread_priority::highest:
        // "Highest" pre-defined priority: We use the policy `SCHED_RR` ("round-robin") with a priority in the middle of the available range.
        policy = SCHED_RR;
        param.sched_priority = sched_get_priority_min(SCHED_RR) + (sched_get_priority_max(SCHED_RR) - sched_get_priority_min(SCHED_RR)) / 2;
        break;
    case os_thread_priority::above_normal:
        // "Above normal" pre-defined priority: We use the policy `SCHED_OTHER` (the default) with the highest possible priority.
        policy = SCHED_OTHER;
        param.sched_priority = sched_get_priority_max(SCHED_OTHER);
        break;
    case os_thread_priority::normal:
        // "Normal" pre-defined priority: We use the policy `SCHED_OTHER` (the default) with a priority in the middle of the available range (which appears to be the default?).
        policy = SCHED_OTHER;
        param.sched_priority = sched_get_priority_min(SCHED_OTHER) + (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) / 2;
        break;
    case os_thread_priority::below_normal:
        // "Below normal" pre-defined priority: We use the policy `SCHED_OTHER` (the default) with a priority equal to 2/3rds of the normal value.
        policy = SCHED_OTHER;
        param.sched_priority = sched_get_priority_min(SCHED_OTHER) + (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) * 2 / 3;
        break;
    case os_thread_priority::lowest:
        // "Lowest" pre-defined priority: We use the policy `SCHED_OTHER` (the default) with a priority equal to 1/3rd of the normal value.
        policy = SCHED_OTHER;
        param.sched_priority = sched_get_priority_min(SCHED_OTHER) + (sched_get_priority_max(SCHED_OTHER) - sched_get_priority_min(SCHED_OTHER)) / 3;
        break;
    case os_thread_priority::idle:
        // "Idle" pre-defined priority on macOS: We use the policy `SCHED_OTHER` (the default) with the lowest possible priority.
        policy = SCHED_OTHER;
        param.sched_priority = sched_get_priority_min(SCHED_OTHER);
        break;
    default:
        return false;
    }
    return pthread_setschedparam(pthread_self(), policy, &param) == 0;
#endif
}

#endif
}