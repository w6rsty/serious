#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#include <array>

namespace serious
{

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions();
};

}