#include "log.h"
#include <gtest/gtest.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <regex>

// void deleteDirectory(const std::filesystem::path& dir);
void deleteFile(const std::filesystem::path& file);
bool fileCompare(const std::filesystem::path& f1, std::string&& pattern);
std::string getPattern();

TEST(Log, FmtTest)
{
    // get current work directory
    auto path = getcwd(NULL,0);
    std::filesystem::path fullPath(path);
    free(path);

    fullPath.append("test/ts_log/test_log");

    Log::GetInstance().Init(fullPath.c_str(), 3, 1);
    LOG_DEBUG("this log will not be recorded");
    LOG_INFO("Start record from here");
    LOG_WARN("Today is {}", "Fri");
    LOG_ERROR("Flush {} times", 1);

    Log::GetInstance().Stop();

    std::string f1 = fmt::format("{}_{:%Y-%m-%d}", fullPath.c_str(), std::chrono::system_clock::now());
    EXPECT_TRUE(fileCompare(f1, getPattern()));
    EXPECT_FALSE(fileCompare(f1, ".*this log will not be recorded.*"));
    deleteFile(f1);
}

// [D] | 2023-09-22:16:31 |this log will not be recorded
// [I] | 2023-09-22:16:31 |Start record from here
// [W] | 2023-09-22:16:31 |Today is Fri
// [E] | 2023-09-22:16:31 |Flush 1 times

std::string getPattern()
{
    return std::string(".*Start record from here.*Today is Fri.*Flush 1 times.*");
}

// void deleteDirectory(const std::filesystem::path& dir)
// {
//     for (const auto& entry : std::filesystem::directory_iterator(dir)) 
//         std::filesystem::remove_all(entry.path());
//     std::filesystem::remove(dir);
// }

void deleteFile(const std::filesystem::path& file)
{
    std::filesystem::remove_all(file);
}

bool fileCompare(const std::filesystem::path& f1, std::string&& pattern)
{
    std::string s1;
    auto getContent = [](auto& file, auto& s)
    {
        std::ifstream op;
        op.open(file);
        while (!op.eof())
        {
            s += op.get();
        }
        op.close();
    };
    
    getContent(f1, s1);
    std::regex reg(pattern, std::regex::extended);
    return std::regex_match(s1, reg);
}