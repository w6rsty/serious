#include "serious/VulkanCommand.hpp"
#include "serious/VulkanDevice.hpp"

namespace serious
{

VulkanCommandPool::VulkanCommandPool(VulkanDevice* device)
    : m_CmdPool(VK_NULL_HANDLE)
    , m_Device(device)
{
}

VulkanCommandPool::~VulkanCommandPool()
{
}

void VulkanCommandPool::Destroy()
{
    if (m_CmdPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_Device->GetHandle(), m_CmdPool, nullptr);
    }
}


void VulkanCommandPool::Create(VulkanQueue* queue)
{
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queue->GetFamilyIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(m_Device->GetHandle(), &poolInfo, nullptr, &m_CmdPool));
}

VulkanCommandBuffer::VulkanCommandBuffer(VulkanDevice* device, const Ref<VulkanCommandPool>& cmdPool)
    : m_CmdBuf(VK_NULL_HANDLE)
    , m_CmdPool(cmdPool)
    , m_Device(device)
{
}

void VulkanCommandBuffer::Allocate()
{
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CmdPool->GetHandle(); 
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(m_Device->GetHandle(), &allocInfo, &m_CmdBuf));
    Info("allocate command buffer");
}

void VulkanCommandBuffer::Free()
{
    vkFreeCommandBuffers(m_Device->GetHandle(), m_CmdPool->GetHandle(), 1, &m_CmdBuf);
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
}

void VulkanCommandBuffer::Begin(VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo cmdBufBegin = {};
    cmdBufBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufBegin.flags = flags;
    VK_CHECK_RESULT(vkBeginCommandBuffer(m_CmdBuf, &cmdBufBegin));
}

void VulkanCommandBuffer::End()
{
    VK_CHECK_RESULT(vkEndCommandBuffer(m_CmdBuf));
}

}
