#include "serious/VulkanCommand.hpp"
#include "serious/VulkanContext.hpp"

namespace serious
{

VulkanCommandPool::VulkanCommandPool()
    : m_GraphicCommandPool(VK_NULL_HANDLE)
{
    Ref<VulkanDevice> device = VulkanContext::Get().GetDevice();
    
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // TODO: assign queue family index
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(device->GetHandle(), &poolInfo, nullptr, &m_GraphicCommandPool));
}

VulkanCommandPool::~VulkanCommandPool()
{
    Ref<VulkanDevice> device = VulkanContext::Get().GetDevice();

    if (m_GraphicCommandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device->GetHandle(), m_GraphicCommandPool, nullptr);
    }
}

VkCommandBuffer VulkanCommandPool::AllocateCommandBuffer()
{
    Ref<VulkanDevice> device = VulkanContext::Get().GetDevice();

    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_GraphicCommandPool; 
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VK_CHECK_RESULT(vkAllocateCommandBuffers(device->GetHandle(), &allocInfo, &commandBuffer)); 

    return commandBuffer;
}

}
