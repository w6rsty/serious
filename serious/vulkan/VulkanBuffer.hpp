#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace serious
{

class VulkanDevice;
struct Vertex;
class VulkanCommandPool;
class VulkanCommandBuffer;
class VulkanQueue;
class VulkanSwapchain;

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
    void MapOnce(const void* data, const VkDeviceSize& size);
    void Map(const void* data, const VkDeviceSize& size);
    void* MapTo(const VkDeviceSize& size);
    void Unmap();
    void Copy(VulkanBuffer& srcBuffer, const VkDeviceSize& size, VulkanCommandBuffer& cmdBuf, VulkanQueue& queue);

    inline VkBuffer GetHandle() const { return m_Buffer; } 

private:
    VkBuffer m_Buffer;
    VulkanDevice* m_Device;
    VkDeviceMemory m_Memory;
};

uint32_t FindMemoryType(VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);

class VulkanDeviceBuffer
{
public:
    VulkanDeviceBuffer(
        VulkanDevice* device,
        VkBufferUsageFlags usage,
        VkDeviceSize size,
        const void* data,
        VulkanCommandPool& tsfCmdPool
    );
    ~VulkanDeviceBuffer();
    void Destroy();

    inline VkBuffer GetHandle() const { return m_DeviceBuffer.GetHandle(); }
private:
    VulkanBuffer m_DeviceBuffer;
};

class VulkanTextureImage final
{
public:
    VulkanTextureImage(
        VulkanDevice* device,
        const VulkanSwapchain& swapchain,
        VkImageLayout initialLayout,
        const std::string& path,
        VulkanCommandPool& cmdPool);
    ~VulkanTextureImage();
    void Destroy();

    inline VkImage     GetHandle() const { return m_Image; }
    inline VkImageView GetView() const { return m_ImageView; }
    inline VkSampler   GetSampler() const { return m_Sampler; }
private:
    void CreateImage(
        uint32_t width,
        uint32_t height,
        VkFormat format,
        VkImageTiling imageTiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);
    void TransitionLayout(VkImageLayout newLayout, VulkanCommandPool& cmdPool);
    void CopyBufferToImage(const VulkanBuffer& buffer, VulkanCommandPool& cmdPool);
    void CreateView(VkFormat format, VkComponentMapping mapping);
    void CreateSampler();
private:
    VkImage m_Image;
    int m_Width, m_Height, m_Channels;
    VulkanDevice* m_Device;
    VkDeviceMemory m_DeviceMemory;
    VkImageLayout m_Layout;
    VkImageView m_ImageView;
    VkSampler m_Sampler;
};

}
