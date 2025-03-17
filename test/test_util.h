#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <thread>
#include <chrono>
#include <gtest/gtest.h>

#define EXPECT_TRUE_FOR_X_MS(time, cond)                                \
    {                                                                   \
        int cnt = 0;                                                    \
        while (cnt < time)                                              \
        {                                                               \
            ++cnt;                                                      \
            if ((cond)) {                                               \
                break;                                                  \
            }                                                           \
            std::this_thread::sleep_for(std::chrono::milliseconds(1));  \
        }                                                               \
        EXPECT_TRUE((cond));                                            \
    }

#endif