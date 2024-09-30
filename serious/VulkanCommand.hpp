#pragma once

#include "serious/VulkanUtils.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanQueue;
class VulkanDevice;

class VulkanCommandPool final
{
public:
    VulkanCommandPool(VulkanDevice* device); 
    ~VulkanCommandPool();

    void Create(VulkanQueue* queue);
    void Destroy();

    inline VkCommandPool GetHandle() const { return m_CmdPool; }
private:
    VkCommandPool m_CmdPool;
    VulkanDevice* m_Device;
};

class VulkanCommandBuffer final
{
public:
    VulkanCommandBuffer(VulkanDevice* device, const Ref<VulkanCommandPool>& cmdPool);
    ~VulkanCommandBuffer();

    void Allocate();
    void Free();

    void Begin(VkCommandBufferUsageFlags flags = 0);
    void End();

    inline VkCommandBuffer GetHandle() const { return m_CmdBuf; }
private:
    VkCommandBuffer m_CmdBuf;
    Ref<VulkanCommandPool> m_CmdPool;
    VulkanDevice* m_Device;
};

}
