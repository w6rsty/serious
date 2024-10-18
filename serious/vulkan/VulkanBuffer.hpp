#pragma once

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanDevice;
struct Vertex;
class VulkanCommandBuffer;
class VulkanQueue;

class VulkanBuffer final
{
public:
    VulkanBuffer(VulkanDevice* device);
    ~VulkanBuffer();
    void Create(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties
    );
    void Destroy();
    void Map(const void* data, const VkDeviceSize& size);
    void Copy(VulkanBuffer& srcBuffer, const VkDeviceSize& size, VulkanCommandBuffer& cmdBuf, VulkanQueue& queue);

    inline VkBuffer GetHandle() const { return m_Buffer; } 
protected:
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);
protected:
    VkBuffer m_Buffer;
    VulkanDevice* m_Device;
    VkDeviceMemory m_Memory;
};

}