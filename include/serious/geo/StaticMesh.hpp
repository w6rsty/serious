#pragma once
#include "serious/graphics/vulkan/Vertex.hpp"
#include <vector>

namespace serious::mesh
{

struct Plane
{
    inline static std::vector<Vertex> vertices = {
        {{-0.5f, 0.0f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{ 0.5f, 0.0f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{ 0.5f, 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.0f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    };

    inline static std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };
};

}