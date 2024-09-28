#pragma once

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanCommandPool final
{
public:
    VulkanCommandPool(); 
    ~VulkanCommandPool();

    VkCommandBuffer AllocateCommandBuffer();

    inline VkCommandPool GetGraphicCommandPool() const { return m_GraphicCommandPool; }
private:
    VkCommandPool m_GraphicCommandPool;
};

}
