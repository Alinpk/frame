#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <tuple>

namespace XH {

#define XH_VERSION_MAJOR 0
#define XH_VERSION_MINOR 1
#define XH_VERSION_PATCH 0

struct version
{
    constexpr version(const std::uint64_t major_, const std::uint64_t minor_, const std::uint64_t patch_) noexcept
        : major(major_), minor(minor_), patch(patch_)
        {}

    [[nodiscard]] constexpr friend bool operator==(const version& lhs, const version& rhs) noexcept
    {
        return lhs.major == rhs.major && lhs.minor == rhs.minor && lhs.patch == rhs.patch;
    }

    [[nodiscard]] constexpr friend bool operator!=(const version& lhs, const version& rhs) noexcept
    {
        return !(lhs == rhs);
    }

    [[nodiscard]] constexpr friend bool operator<(const version& lhs, const version& rhs) noexcept
    {
        return std::tie(lhs.major, lhs.minor, lhs.patch) < std::tie(rhs.major, rhs.minor, rhs.patch);
    }

    [[nodiscard]] constexpr friend bool operator>(const version& lhs, const version& rhs) noexcept
    {
        return rhs < lhs;
    }

    [[nodiscard]] constexpr friend bool operator<=(const version& lhs, const version& rhs) noexcept
    {
        return !(rhs < lhs);
    }

    [[nodiscard]] constexpr friend bool operator>=(const version& lhs, const version& rhs) noexcept
    {
        return !(lhs < rhs);
    }

    [[nodiscard]] std::string to_string() const
    {
        return std::to_string(major) + '.' + std::to_string(minor) + '.' + std::to_string(patch);
    }

    [[nodiscard]] constexpr friend std::ostream& operator<<(std::ostream& os, const version& v) noexcept
    {
        return os << v.major << '.' << v.minor << '.' << v.patch;
    }

    std::uint64_t major;
    std::uint64_t minor;
    std::uint64_t patch;
};


inline constexpr version current_version = { XH_VERSION_MAJOR, XH_VERSION_MINOR, XH_VERSION_PATCH };
}