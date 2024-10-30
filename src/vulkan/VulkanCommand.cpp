#include "serious/vulkan/VulkanCommand.hpp"

namespace serious
{

VulkanCommandBuffer VulkanCommandPool::Allocate()
{
    VulkanCommandBuffer cmdBuf;

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CmdPool; 
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device, &allocInfo, &cmdBuf.m_CmdBuf));

    return cmdBuf;
}

void VulkanCommandPool::Free(VulkanCommandBuffer& cmdBuf)
{
    vkFreeCommandBuffers(m_Device, m_CmdPool, 1, &cmdBuf.m_CmdBuf);
}

VulkanCommandBuffer::VulkanCommandBuffer()
    : m_CmdBuf(VK_NULL_HANDLE)
{
}

void VulkanCommandBuffer::BeginRenderPass(const VkRenderPassBeginInfo& renderPassInfo, VkSubpassContents contents)
{
    vkCmdBeginRenderPass(m_CmdBuf, &renderPassInfo, contents);
}

void VulkanCommandBuffer::EndRenderPass()
{
    vkCmdEndRenderPass(m_CmdBuf);
}

void VulkanCommandBuffer::NextSubpass(VkSubpassContents contents)
{
    vkCmdNextSubpass(m_CmdBuf, contents);
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo cmdBufBegin {};
    cmdBufBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBegin.flags = flags;
    VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuf, &cmdBufBegin));
}

void VulkanCommandBuffer::BeginSingle()
{
    VkCommandBufferBeginInfo cmdBufBegin {};
    cmdBufBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBegin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuf, &cmdBufBegin));
}

void VulkanCommandBuffer::End()
{
    VK_CHECK_RESULT(vkEndCommandBuffer(m_CmdBuf));
}

void VulkanCommandBuffer::Reset()
{
    VK_CHECK_RESULT(vkResetCommandBuffer(m_CmdBuf, 0));
}

void VulkanCommandBuffer::BindGraphicsPipeline(VkPipeline pipeline)
{
    vkCmdBindPipeline(m_CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
}

void VulkanCommandBuffer::BindVertexBuffer(VkBuffer buffer, uint32_t offset)
{
    VkBuffer vertexBuffer[] = {buffer};
    VkDeviceSize dataOffset[] = {offset};
    vkCmdBindVertexBuffers(m_CmdBuf, 0, 1, vertexBuffer, dataOffset);
}

void VulkanCommandBuffer::BindIndexBuffer(VkBuffer buffer, uint32_t offset, VkIndexType type)
{
    vkCmdBindIndexBuffer(m_CmdBuf, buffer, offset, type);
}


void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_CmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_CmdBuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void VulkanCommandBuffer::PipelineMemoryBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkMemoryBarrier* memory)
{
    vkCmdPipelineBarrier(m_CmdBuf, srcStageMask, dstStageMask, 0, 1, memory, 0, nullptr, 0, nullptr);
}

void VulkanCommandBuffer::PipelineBufferBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkBufferMemoryBarrier* bufferMemory)
{
    vkCmdPipelineBarrier(m_CmdBuf, srcStageMask, dstStageMask, 0, 0, nullptr, 1, bufferMemory, 0, nullptr);
}

void VulkanCommandBuffer::PipelineImageBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, const VkImageMemoryBarrier* imageMemory)
{
    vkCmdPipelineBarrier(m_CmdBuf, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, imageMemory);
}

void VulkanCommandBuffer::CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize offset)
{
    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = offset;
    copyRegion.dstOffset = offset;
    copyRegion.size = size;

    vkCmdCopyBuffer(m_CmdBuf, src, dst, 1, &copyRegion);
}

void VulkanCommandBuffer::CopyBufferToImage(VkBuffer buffer, VkImage image, const VkBufferImageCopy* region)
{
    vkCmdCopyBufferToImage(m_CmdBuf, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, region);
}

void VulkanCommandBuffer::BindDescriptorSet(VkPipelineLayout layout, const VkDescriptorSet& descriptorSet)
{
    vkCmdBindDescriptorSets(m_CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &descriptorSet, 0, nullptr);
}

void VulkanCommandBuffer::SubmitOnceTo(VulkanQueue& queue, VkFence fence)
{
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CmdBuf;
    
    queue.Submit(submitInfo, fence);
    queue.WaitIdle();
}


}
