#pragma once

#include <chrono> // IWYU pragma: keep

#include <fmt/core.h>
#include <fmt/color.h>

namespace serious
{

#define SEInfo(...) \
    do { \
        auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now()); \
        fmt::println("[{}] {}", std::format("{}", now), fmt::format(__VA_ARGS__)); \
    } while (0)

#define SETrace(...) \
    do { \
        auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now()); \
        fmt::print(fmt::fg(fmt::color::green), "[{}] In ({}) {}\n", std::format("{}", now), __FUNCTION__, fmt::format(__VA_ARGS__)); \
    } while (0)

#define SEWarn(...) \
    do { \
        auto now = std::chrono::current_zone()->to_local(std::chrono::system_clock::now()); \
        fmt::print(fmt::fg(fmt::color::peru), "[{}] {}\n", std::format("{}", now), fmt::format(__VA_ARGS__)); \
    } while (0)

#define SEError(...) \
    do { \
        fmt::print(fmt::fg(fmt::color::red), "{} At {} {}\n", fmt::format(__VA_ARGS__), __FILE__, __LINE__); \
    } while (0)

#define SEFatal(...) \
    do { \
        fmt::print(fmt::fg(fmt::color::red), "{} At {} {} \n", fmt::format(__VA_ARGS__), __FILE__, __LINE__); \
        std::terminate(); \
    } while (0)

}