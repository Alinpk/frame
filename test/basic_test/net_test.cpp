#include <gtest/gtest.h>
#include "basic/net.h"
#include <regex>

namespace XH::TEST
{
    TEST(NetTest, GetLocalIPWithBoost)
    {
        std::regex ipv4Regex(R"(^(?!127\.0\.0\.1$)(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
        auto local_ip = getLocalIPWithBoost();
        EXPECT_TRUE(!!local_ip);
        EXPECT_TRUE(std::regex_match(local_ip.value(), ipv4Regex));
    }
} // namespace XHs