#pragma once

#include <fmt/core.h>
#include <fmt/color.h>

#include <string>

namespace serious {

std::string ReadFile(const std::string& filename);

/// Terminate program with message
#define FatalError(...) \
fmt::print(fmt::fg(fmt::color::red), __VA_ARGS__); \
fmt::print("\n"); \
std::terminate()

#define Warn(...) \
fmt::print(fmt::fg(fmt::color::peru), __VA_ARGS__); \
fmt::print("\n")

#define Info(...) \
fmt::print(fmt::fg(fmt::color::teal), __VA_ARGS__); \
fmt::print("\n")

}
