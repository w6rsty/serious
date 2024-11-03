#pragma once
#include "serious/io/log.hpp"

#include <string>
#include <fstream>

namespace serious
{

static std::string ReadFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        SEWarn("failed to open file: {}", filename);
        return "";
    }

    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0);

    std::string buffer(size, 0);
    file.read(buffer.data(), static_cast<std::streamsize>(size));

    file.close();

    return buffer;
}

}