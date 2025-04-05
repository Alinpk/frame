#include "basic/thread_pool.h"
#include "test_util.h"
#include <gtest/gtest.h>
#include <future>
#include <numeric>
#include <thread>
#include <vector>
namespace XH::TEST {

TEST(ThreadPoolTester, ThreadPoolTest)
{
    std::vector<int> test_data{1,2,4,5,6,9,7,8,10,11,12,13,14,15,16};
    int sum = std::accumulate(test_data.begin(), test_data.end(), 0);

    constexpr size_t test_thread_count = 4;
    std::vector<int> thread_id;
    std::vector<std::future<int> > futures;
    std::mutex mtx;
    XH::thread_job_pool_t pool(test_thread_count, [](size_t){}, [&]{
        std::promise<int> prom;
        {
            std::lock_guard<std::mutex> lock(mtx);
            auto idx = XH::this_thread::get_index();

            thread_id.push_back(idx.has_value() ? idx.value() : -1);
            futures.push_back(prom.get_future());
        }
        prom.set_value(std::accumulate(test_data.begin(), test_data.end(), 0));
    });
    EXPECT_TRUE_FOR_X_MS(500, thread_id.size() == test_thread_count);
    for (auto& fut : futures) {
        EXPECT_EQ(fut.get(), sum);
    }
}

}