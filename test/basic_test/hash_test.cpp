#include "basic/hash.h"
#include <gtest/gtest.h>

namespace XH::TEST {
TEST(Hash, MurmurHash3_32)
{
    std::string key = "Hello, World!";
    uint32_t seed = 0;
    uint32_t val1 = MurmurHash3_32<std::string, 1>(key, seed);
    
    key += "tail";
    uint32_t val2 = MurmurHash3_32<std::string, 1>(key, seed);
    uint32_t val3 = MurmurHash3_32<std::string, 1>(key, seed);

    EXPECT_NE(val1, val2);
    EXPECT_EQ(val2, val3);
}
}