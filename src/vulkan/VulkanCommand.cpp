#include "serious/vulkan/VulkanCommand.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanBuffer.hpp"
#include <vulkan/vulkan_core.h>

namespace serious
{

VulkanCommandPool::VulkanCommandPool(VulkanDevice* device, VulkanQueue* queue)
    : m_CmdPool(VK_NULL_HANDLE)
    , m_Device(device)
    , m_Queue(queue)
{
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_Queue->GetFamilyIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(m_Device->GetHandle(), &poolInfo, nullptr, &m_CmdPool));
}

VulkanCommandPool::~VulkanCommandPool()
{
}

void VulkanCommandPool::Destroy()
{
    vkDestroyCommandPool(m_Device->GetHandle(), m_CmdPool, nullptr);
}

VulkanCommandBuffer VulkanCommandPool::Allocate()
{
    VulkanCommandBuffer cmdBuf;

    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CmdPool; 
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device->GetHandle(), &allocInfo, &cmdBuf.m_CmdBuf));

    return cmdBuf;
}

void VulkanCommandPool::Free(VulkanCommandBuffer& cmdBuf)
{
    vkFreeCommandBuffers(m_Device->GetHandle(), m_CmdPool, 1, &cmdBuf.m_CmdBuf);
}

VulkanCommandBuffer::VulkanCommandBuffer()
    : m_CmdBuf(VK_NULL_HANDLE)
{
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo cmdBufBegin {};
    cmdBufBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBegin.flags = flags;
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

void VulkanCommandBuffer::BindGraphicsPipeline(const VulkanPipeline& pipeline)
{
    vkCmdBindPipeline(m_CmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.GetHandle());
}

void VulkanCommandBuffer::BindVertexBuffer(const VulkanBuffer& buffer, uint32_t offset)
{
    VkBuffer vertexBuffer[] = {buffer.GetHandle()};
    VkDeviceSize dataOffset[] = {offset};
    vkCmdBindVertexBuffers(m_CmdBuf, 0, 1, vertexBuffer, dataOffset);
}

void VulkanCommandBuffer::BindIndexBuffer(const VulkanBuffer& buffer, uint32_t offset, VkIndexType type)
{
    vkCmdBindIndexBuffer(m_CmdBuf, buffer.GetHandle(), offset, type);
}


void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    vkCmdDraw(m_CmdBuf, vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    vkCmdDrawIndexed(m_CmdBuf, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}


void VulkanCommandBuffer::CopyBuffer(const VulkanBuffer& srcBuf, const VulkanBuffer& dstBuf, const VkDeviceSize& size)
{
    VkBufferCopy copyRegion {};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;

    vkCmdCopyBuffer(m_CmdBuf, srcBuf.GetHandle(), dstBuf.GetHandle(), 1, &copyRegion);
}

}
