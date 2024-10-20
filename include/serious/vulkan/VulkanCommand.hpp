#pragma once

#include "serious/vulkan/VulkanPipeline.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanCommandPool;
class VulkanQueue;
class VulkanDevice;
class VulkanBuffer;

class VulkanCommandBuffer final
{
public:
    VulkanCommandBuffer();

    void Begin(VkCommandBufferUsageFlags flags);
    void BeginSingle();
    void End();
    void Reset();
    void BindGraphicsPipeline(const VulkanPipeline& pipeline);
    void BindVertexBuffer(VkBuffer buffer, uint32_t offset);
    void BindIndexBuffer(VkBuffer buffer, uint32_t offset, VkIndexType type);
    void BindDescriptorSet(VkPipelineLayout layout, const VkDescriptorSet& descriptorSet);
    void CopyBuffer(const VulkanBuffer& srcBuf, const VulkanBuffer& dstBuf, const VkDeviceSize& size);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy* region);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void PipelineMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier* memory);
    void PipelineBufferBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier* bufferMemory);
    void PipelineImageBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier* imageMemory);
    // Submit one command buffer without synchronization objects
    void SubmitOnceTo(VulkanQueue& queue);

    inline VkCommandBuffer GetHandle() const { return m_CmdBuf; }
private:
    VkCommandBuffer m_CmdBuf;

    friend class VulkanCommandPool;
};

class VulkanCommandPool final
{
public:
    VulkanCommandPool(VulkanDevice* device, VulkanQueue* queue); 
    ~VulkanCommandPool();
    void Destroy();
    
    VulkanCommandBuffer Allocate();
    void Free(VulkanCommandBuffer& cmdBuf);

    inline VkCommandPool GetHandle() const { return m_CmdPool; }
private:
    VkCommandPool m_CmdPool;
    VulkanDevice* m_Device;
    VulkanQueue* m_Queue;
};

}
