#pragma once
#include "basic/thread.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>

namespace XH {

#ifdef __cpp_lib_move_only_function
template <typename... S>
using function_t = std::move_only_function<S...>
#else
template <typename... S>
using function_t = std::function<S...>;
#endif

#ifdef __cpp_exceptions
    struct wait_deadlock : public std::runtime_error
{
    wait_deadlock() : std::runtime_error("wait_deadlock") {};
};
#endif
using task_t = function_t<void()>;

using opt_t = uint8_t;
enum class topt_t : opt_t
{
    none = 0,
    pause = 1 << 0,
    priority = 1 << 1,
    deadlock_detect = 1 << 2,
};

constexpr opt_t operator&(const topt_t lhs, const topt_t rhs) noexcept
{
    return static_cast<opt_t>(static_cast<opt_t>(lhs) & static_cast<opt_t>(rhs));
}

constexpr topt_t operator|(const topt_t lhs, const topt_t rhs) noexcept
{
    return static_cast<topt_t>(static_cast<opt_t>(lhs) | static_cast<opt_t>(rhs));
}

#ifdef __cpp_lib_jthread
    #define THREAD_POOL_WORKER_TOKEN const std::stop_token &stop_token,
    #define THREAD_POOL_WAIT_TOKEN stop_token,
    #define THREAD_POOL_OR_STOP_CONDITION stop_token.stop_requested() ||
    #define THREAD_POOL_STOP_CONDITION stop_token.stop_requested()
#else
    #define THREAD_POOL_WORKER_TOKEN
    #define THREAD_POOL_WAIT_TOKEN
    #define THREAD_POOL_OR_STOP_CONDITION !m_workers_running ||
    #define THREAD_POOL_STOP_CONDITION !m_workers_running
#endif

template <topt_t options = topt_t::none>
class thread_pool
{
public:
    thread_pool() : thread_pool(0, [] {}) {}

    explicit thread_pool(const std::size_t num_threads) : thread_pool(num_threads, [] {}) {}

#define THREAD_POOL_INIT_FUNC_CONCEPT(F) typename F, typename = std::enable_if_t<std::is_invocable_v<F> || std::is_invocable_v<F, std::size_t> >

    template <THREAD_POOL_INIT_FUNC_CONCEPT(F)>
    thread_pool(const std::size_t num_threads, F&& init_func)
    {
        create_threads(num_threads, std::forward<F>(init_func));
    }

    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    thread_pool& operator=(const thread_pool&) = delete;
    thread_pool& operator=(thread_pool&&) = delete;

    ~thread_pool() noexcept
    {
#ifdef __cpp_exceptions
        try
        {
#endif
            // wait();
            m_tasks_available_cv.notify_all();
#ifndef __cpp_lib_jthread
            destroy_threads();
#else
        m_threads.reset();
#endif
#ifdef __cpp_exceptions
        }
        catch (...)
        {
        }
#endif
    }

#ifndef __cpp_lib_jthread
    /**
     * @brief Destroy the threads in the pool.
     */
    void destroy_threads()
    {
        m_workers_running = false;
        {
            const std::scoped_lock tasks_lock(m_tasks_mutex);
        }
        m_tasks_available_cv.notify_all();
        for (std::size_t i = 0; i < m_threads_count; ++i)
            m_threads[i].join();
    }
#endif

    void wait()
    {
#ifdef __cpp_exceptions
        if constexpr (deadlock_detect_enabled)
        {
            if (this_thread::get_pool() == this)
                throw wait_deadlock();
        }
#endif
        std::unique_lock tasks_lock(m_tasks_mutex);
        m_waiting = true;
        m_tasks_done_cv.wait(tasks_lock,
            [this]
            {
                if constexpr (pause_enabled)
                    return (m_tasks_running == 0) && (m_paused || m_tasks.empty());
                else
                    return (m_tasks_running == 0) && m_tasks.empty();
            });
        m_waiting = false;
    }

    template <typename F, typename R = std::enable_if_t<std::is_invocable_v<F> || std::is_invocable_v<F, std::size_t>>>
    void set_cleanup_func(F&& cleanup)
    {
        if constexpr (std::is_invocable_v<F, std::size_t>)
        {
            m_cleanup_func = std::forward<F>(cleanup);
        }
        else
        {
            m_cleanup_func = [cleanup = std::forward<F>(cleanup)](std::size_t)
            {
                cleanup();
            };
        }
    }

    void submit_task(task_t&& task)
    {
        std::unique_lock tasks_lock(m_tasks_mutex);
        m_tasks.push(std::move(task));
        m_tasks_available_cv.notify_one();
    }

private:
    template <typename F>
    void create_threads(const std::size_t num_threads, F&& init)
    {
        if constexpr (std::is_invocable_v<F, std::size_t>)
        {
            m_init_func = std::forward<F>(init);
        }
        else
        {
            m_init_func = [init = std::forward<F>(init)](std::size_t)
            {
                init();
            };
        }
        m_threads_count = determine_num_threads(num_threads);
        m_threads = std::make_unique<thread_t[]>(m_threads_count);

        {
            std::unique_lock lock(m_tasks_mutex);
            m_tasks_running = m_threads_count;
#ifndef __cpp_lib_jthread
            m_workers_running = true;
#endif
        }
        for (std::size_t i = 0; i < m_threads_count; ++i)
        {
            m_threads[i] = thread_t(
                [this, i]
#ifdef __cpp_lib_jthread
                (const std::stop_token& stop_token)
#endif
                {
                    worker(THREAD_POOL_WAIT_TOKEN i);
                });
        }
    }

    [[nodiscard]] std::size_t determine_num_threads(std::size_t num_threads) const
    {
        if (num_threads == 0)
        {
            return std::thread::hardware_concurrency();
        }
        else if (num_threads > 0)
        {
            return num_threads;
        }
        else
        {
            return 1;
        }
    }

    [[nodiscard]] task_t pop_task()
    {
        task_t task;
        task = std::move(m_tasks.front());
        m_tasks.pop();
        return task;
    }

    void worker(THREAD_POOL_WORKER_TOKEN const std::size_t idx)
    {
        this_thread::m_index = idx;
        this_thread::m_pool = this;
        m_init_func(idx);

        while (true)
        {
            std::unique_lock tasks_lock(m_tasks_mutex);
            --m_tasks_running;
            bool paused;
            if constexpr (pause_enabled)
                paused = m_paused;
            else
                paused = false;

            if (m_waiting && (m_tasks_running == 0) && (paused || m_tasks.empty()))
            {
                m_tasks_done_cv.notify_all();
            }
            m_tasks_available_cv.wait(tasks_lock, THREAD_POOL_WAIT_TOKEN
                [THREAD_POOL_WAIT_TOKEN this, paused]
                {
                    return THREAD_POOL_OR_STOP_CONDITION !(paused || m_tasks.empty());
                });

            if (THREAD_POOL_STOP_CONDITION)
            {
                break;
            }

            task_t task = pop_task();
            ++m_tasks_running;
            tasks_lock.unlock();

#ifdef __cpp_exception
            try
            {
                task();
            }
            catch (...)
            {
                // Handle exception
            }
#else
            task();
#endif
        }
        m_cleanup_func(idx);
        this_thread::m_index = std::nullopt;
        this_thread::m_pool = std::nullopt;
    }

private:
    static constexpr bool pause_enabled = !!(options & topt_t::pause);
    static constexpr bool deadlock_detect_enabled = !!(options & topt_t::deadlock_detect);

    // TODO
    static constexpr bool priority_enabled = !!(options & topt_t::priority);

private:
    //@brief An initialization function that is called when a thread is created.
    function_t<void(std::size_t)> m_init_func = [](std::size_t) {};
    //@brief A function that is called when a thread is stopped.
    function_t<void(std::size_t)> m_cleanup_func = [](std::size_t) {};

    std::mutex m_tasks_mutex;
    std::size_t m_threads_count = 0;
    std::atomic<std::size_t> m_tasks_running = 0;
    bool m_waiting = false;
    std::conditional_t<pause_enabled, bool, std::monostate> m_paused = {};

    std::unique_ptr<thread_t[]> m_threads = nullptr;
    std::conditional_t<priority_enabled, std::priority_queue<task_t>, std::queue<task_t>> m_tasks = {};

    std::condition_variable m_tasks_done_cv;
    std::condition_variable_any m_tasks_available_cv;

#ifndef __cpp_lib_jthread
    bool m_workers_running = false;
#endif
};

using base_thread_pool_t = thread_pool<topt_t::none>;
} // namespace XH