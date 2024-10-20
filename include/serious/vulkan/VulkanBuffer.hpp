#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace serious
{

struct Vertex;
class VulkanDevice;
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

void CreateImage(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling imageTiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory,
    VulkanDevice& device
);

void CreateImageView(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkComponentMapping mapping,
    VkImageView& imageView,
    VkDevice device
);

void TransitionLayout(
    VkImage image,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkImageAspectFlags aspectFlags,
    VulkanCommandPool& cmdPool,
    VulkanQueue& gfxQueue
);

class VulkanTextureImage final
{
public:
    VulkanTextureImage(
        VulkanDevice* device,
        const VulkanSwapchain& swapchain,
        const std::string& path,
        VulkanCommandPool& cmdPool);
    ~VulkanTextureImage();
    void Destroy();

    inline VkImage     GetHandle() const { return m_Image; }
    inline VkImageView GetView() const { return m_ImageView; }
    inline VkSampler   GetSampler() const { return m_Sampler; }
private:
    void CopyBufferToImage(const VulkanBuffer& buffer, VulkanCommandPool& cmdPool);
    void CreateSampler();
private:
    VkImage m_Image;
    VulkanDevice* m_Device;
    int m_Width;
    int m_Height;
    VkDeviceMemory m_Memory;
    VkImageView m_ImageView;
    VkSampler m_Sampler;
};

class VulkanDepthImage final
{
public:
    VulkanDepthImage(
        VulkanDevice* device,
        const VulkanSwapchain& swapchain,
        VulkanCommandPool& cmdPool);
    ~VulkanDepthImage();
    void Destroy();

    inline VkImage     GetHandle() const { return m_Image; }
    inline VkImageView GetView() const { return m_ImageView; }
private:
    VkImage m_Image;
    VulkanDevice* m_Device;
    VkDeviceMemory m_Memory;
    VkImageView m_ImageView;
    VkFormat m_DepthFormat;
};

}
