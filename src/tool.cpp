#include "serious/tool.hpp"

#include <iostream>
#include <fstream>

namespace serious {

std::string ReadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return "";
    }

    size_t size = file.tellg();
    file.seekg(0);

    std::string buffer(size, 0);
    file.read(buffer.data(), size);

    file.close();

    return std::move(buffer);
}

}