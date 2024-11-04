#pragma once

#include <vector>

#include <vulkan/vulkan.h>

namespace serious
{

class DescriptorAllocator
{
public:

private:
    VkDevice device;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    VkDescriptorPool m_DescriptorPool;
};

}