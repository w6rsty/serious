#pragma once

#include "serious/vulkan/VulkanPipeline.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanQueue;
class VulkanDevice;

class VulkanCommandPool final
{
public:
    VulkanCommandPool(VulkanDevice* device, VulkanQueue* queue); 
    ~VulkanCommandPool();
    void Destroy();

    inline VkCommandPool GetHandle() const { return m_CmdPool; }
private:
    VkCommandPool m_CmdPool;
    VulkanDevice* m_Device;
    VulkanQueue* m_Queue;

    friend class VulkanCommandBuffer;
};

class VulkanCommandBuffer final
{
public:
    VulkanCommandBuffer(VulkanDevice* device, VulkanCommandPool* cmdPool);
    ~VulkanCommandBuffer();

    void Allocate();
    void Free();

    void Begin(VkCommandBufferUsageFlags flags = 0);
    void End();
    void Reset();
    void BindGraphicsPipeline(const VulkanPipeline& pipeline);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);

    inline VkCommandBuffer GetHandle() const { return m_CmdBuf; }
private:
    VkCommandBuffer m_CmdBuf;
    VulkanCommandPool* m_CmdPool;
    VulkanDevice* m_Device;
};

}
