#include "serious/vulkan/VulkanBuffer.hpp"
#include "serious/VulkanUtils.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanCommand.hpp"

namespace serious
{

VulkanBuffer::VulkanBuffer(VulkanDevice* device)
    : m_Buffer(VK_NULL_HANDLE)
    , m_Device(device)
{
}

VulkanBuffer::~VulkanBuffer()
{
}

void VulkanBuffer::Create(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties
)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateBuffer(m_Device->GetHandle(), &bufferInfo, nullptr, &m_Buffer));

    VkMemoryRequirements memoryRequirements {};
    vkGetBufferMemoryRequirements(m_Device->GetHandle(), m_Buffer, &memoryRequirements);
    
    VkMemoryAllocateInfo allocateInfo {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

    VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetHandle(), &allocateInfo, nullptr, &m_Memory));
    vkBindBufferMemory(m_Device->GetHandle(), m_Buffer, m_Memory, 0);
}

void VulkanBuffer::Destroy()
{
    vkDestroyBuffer(m_Device->GetHandle(), m_Buffer, nullptr);
    vkFreeMemory(m_Device->GetHandle(), m_Memory, nullptr);
}

void VulkanBuffer::Map(const void* data, const VkDeviceSize& size)
{
    void* mappedData;
    vkMapMemory(m_Device->GetHandle(), m_Memory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, (size_t) size);
    vkUnmapMemory(m_Device->GetHandle(), m_Memory);
    Info("Buffer memory allocated: {} bytes", size);
}

void VulkanBuffer::Copy(VulkanBuffer& srcBuffer, const VkDeviceSize& size, VulkanCommandBuffer& cmdBuf, VulkanQueue& queue)
{
    cmdBuf.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    cmdBuf.CopyBuffer(srcBuffer, *this, size);
    cmdBuf.End();

    VkCommandBuffer cmd = cmdBuf.GetHandle();

    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;

    queue.Submit(submitInfo, VK_NULL_HANDLE);
    queue.WaitIdle();
}


uint32_t VulkanBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties {};
    vkGetPhysicalDeviceMemoryProperties(m_Device->GetGpuHandle(), &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) {
            return i;
        }
    }
    Error("failed to find suitable memory type");
    return 0;   
}

}