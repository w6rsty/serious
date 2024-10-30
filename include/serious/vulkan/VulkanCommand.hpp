#pragma once

#include "serious/vulkan/VulkanDevice.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanCommandBuffer final
{
public:
    VulkanCommandBuffer();

    void BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo, VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
    void EndRenderPass();
    void NextSubpass(VkSubpassContents contents);
    void Begin(VkCommandBufferUsageFlags flags);
    // Begin recording a command buffer for a single use, no need to reset
    void BeginSingle();
    void End();
    void Reset();
    void BindGraphicsPipeline(VkPipeline pipeline);
    void BindVertexBuffer(VkBuffer buffer, uint32_t offset);
    void BindIndexBuffer(VkBuffer buffer, uint32_t offset, VkIndexType type);
    void BindDescriptorSet(VkPipelineLayout layout, const VkDescriptorSet& descriptorSet);
    void CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
    void CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy* region);
    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    void PipelineMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier* memory);
    void PipelineBufferBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier* bufferMemory);
    void PipelineImageBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier* imageMemory);
    void SubmitOnceTo(VulkanQueue& queue, VkFence fence = VK_NULL_HANDLE);

    inline VkCommandBuffer GetHandle() const { return m_CmdBuf; }
private:
    VkCommandBuffer m_CmdBuf;

    friend class VulkanCommandPool;
};

class VulkanCommandPool final
{
public:
    VulkanCommandBuffer Allocate();
    void Free(VulkanCommandBuffer& cmdBuf);
private:
    VkCommandPool m_CmdPool;
    VkDevice m_Device;

    friend class VulkanDevice;
};

}
