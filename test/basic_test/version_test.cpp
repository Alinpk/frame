#include "basic/version.h"
#include <gtest/gtest.h>

namespace XH::TEST {

TEST(VersionTest, compare)
{
    {
        version v(XH_VERSION_MAJOR, XH_VERSION_MINOR, XH_VERSION_PATCH);
        EXPECT_TRUE(v == current_version);
    }

    {
        version min_version(0, 0, 0);
        EXPECT_TRUE(min_version <= current_version);

        version max_version(UINT64_MAX, UINT64_MAX, UINT64_MAX);
        EXPECT_TRUE(max_version >= current_version);
    }
}

}