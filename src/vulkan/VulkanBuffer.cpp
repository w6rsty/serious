#include "serious/vulkan/VulkanBuffer.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanCommand.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/VulkanUtils.hpp"

#include <stb_image.h>

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
    VkMemoryPropertyFlags properties)
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
    allocateInfo.memoryTypeIndex = FindMemoryType(*m_Device, memoryRequirements.memoryTypeBits, properties);

    VK_CHECK_RESULT(vkAllocateMemory(m_Device->GetHandle(), &allocateInfo, nullptr, &m_Memory));
    vkBindBufferMemory(m_Device->GetHandle(), m_Buffer, m_Memory, 0);
}

void VulkanBuffer::Destroy()
{
    vkDestroyBuffer(m_Device->GetHandle(), m_Buffer, nullptr);
    vkFreeMemory(m_Device->GetHandle(), m_Memory, nullptr);
}

void VulkanBuffer::MapOnce(const void* data, const VkDeviceSize& size)
{
    void* mappedData;
    vkMapMemory(m_Device->GetHandle(), m_Memory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, (size_t) size);
    vkUnmapMemory(m_Device->GetHandle(), m_Memory);
}

void VulkanBuffer::Map(const void* data, const VkDeviceSize& size)
{
    void* mappedData;
    vkMapMemory(m_Device->GetHandle(), m_Memory, 0, size, 0, &mappedData);
    memcpy(mappedData, data, (size_t) size);
}

void* VulkanBuffer::MapTo(const VkDeviceSize& size)
{
    void* data;
    vkMapMemory(m_Device->GetHandle(), m_Memory, 0, size, 0, &data);
    return data;
}

void VulkanBuffer::Unmap()
{
    vkUnmapMemory(m_Device->GetHandle(), m_Memory);
}

void VulkanBuffer::Copy(VulkanBuffer& srcBuffer, const VkDeviceSize& size, VulkanCommandBuffer& cmdBuf, VulkanQueue& queue)
{
    cmdBuf.BeginSingle();
    cmdBuf.CopyBuffer(srcBuffer, *this, size);
    cmdBuf.End();
    cmdBuf.SubmitOnceTo(queue);    
    queue.WaitIdle();
}

uint32_t FindMemoryType(VulkanDevice& device, uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties {};
    vkGetPhysicalDeviceMemoryProperties(device.GetGpuHandle(), &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) && memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) {
            return i;
        }
    }
    Error("failed to find suitable memory type");
    return 0;   
}

VulkanDeviceBuffer::VulkanDeviceBuffer(
    VulkanDevice* device,
    VkBufferUsageFlags usage,
    VkDeviceSize size,
    const void* data,
    VulkanCommandPool& tsfCmdPool)
    : m_DeviceBuffer(device)
{
    VulkanBuffer stagingBuffer(device);
    stagingBuffer.Create(
        size, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,   
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    );
    stagingBuffer.MapOnce(data, size);
    m_DeviceBuffer.Create(size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VulkanCommandBuffer cmdBuf = tsfCmdPool.Allocate();
    cmdBuf.BeginSingle();
    cmdBuf.CopyBuffer(stagingBuffer, m_DeviceBuffer, size);
    cmdBuf.End();
    cmdBuf.SubmitOnceTo(*device->GetTransferQueue());
    tsfCmdPool.Free(cmdBuf);
    stagingBuffer.Destroy();
}

VulkanDeviceBuffer::~VulkanDeviceBuffer()
{
}

void VulkanDeviceBuffer::Destroy()
{
    m_DeviceBuffer.Destroy();
}

void CreateImage(
    uint32_t width,
    uint32_t height,
    VkFormat format,
    VkImageTiling imageTiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& imageMemory,
    VulkanDevice& device)
{
    VkImageCreateInfo imageInfo {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = imageTiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    VkDevice deviceHandle = device.GetHandle();
    VK_CHECK_RESULT(vkCreateImage(deviceHandle, &imageInfo, nullptr, &image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(deviceHandle, image, &memRequirements);

    VkMemoryAllocateInfo allocateInfo {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = FindMemoryType(device, memRequirements.memoryTypeBits, properties);
    VK_CHECK_RESULT(vkAllocateMemory(deviceHandle, &allocateInfo, nullptr, &imageMemory));

    vkBindImageMemory(deviceHandle, image, imageMemory, 0);
}

void CreateImageView(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkComponentMapping mapping,
    VkImageView& imageView,
    VkDevice device)
{
    VkImageViewCreateInfo viewInfo {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.components = mapping;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    VK_CHECK_RESULT(vkCreateImageView(device, &viewInfo, nullptr, &imageView));
}

void TransitionLayout(
    VkImage image,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkImageAspectFlags aspectFlags,
    VulkanCommandPool& cmdPool,
    VulkanQueue& gfxQueue)
{
    VulkanCommandBuffer cmdBuf = cmdPool.Allocate();
    cmdBuf.BeginSingle();

    VkImageMemoryBarrier barrier {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = srcLayout;
    barrier.newLayout = dstLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectFlags;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags srcStage = 0;
    VkPipelineStageFlags dstStage = 0;
    if ((srcLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if ((srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) && (dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)){
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if ((srcLayout == VK_IMAGE_LAYOUT_UNDEFINED) && (dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        Error("Unsupported layout transition");
    }
    cmdBuf.PipelineImageBarrier(srcStage, dstStage, &barrier);

    cmdBuf.End();
    cmdBuf.SubmitOnceTo(gfxQueue);
    cmdPool.Free(cmdBuf);
}

VulkanTextureImage::VulkanTextureImage(
    VulkanDevice* device,
    const VulkanSwapchain& swapchain,
    const std::string& path,
    VulkanCommandPool& cmdPool)
    : m_Image(VK_NULL_HANDLE)
    , m_Device(device)
    , m_Width(0)
    , m_Height(0)
    , m_Memory(VK_NULL_HANDLE)
    , m_ImageView(VK_NULL_HANDLE)
{
    int texChannels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &m_Width, &m_Height, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        Error("Failed to load image {}", path);
    }
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(m_Width * m_Height * 4);

    VulkanBuffer stagingBuffer(m_Device);
    stagingBuffer.Create(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.MapOnce(pixels, imageSize);

    stbi_image_free(pixels);

    CreateImage(
        static_cast<uint32_t>(m_Width),
        static_cast<uint32_t>(m_Height),
        swapchain.GetColorFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_Memory,
        *m_Device       
    );

    TransitionLayout(
        m_Image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        cmdPool,
        *m_Device->GetGraphicsQueue()
    );
    CopyBufferToImage(stagingBuffer, cmdPool);
    TransitionLayout(
        m_Image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT,
        cmdPool,
        *m_Device->GetGraphicsQueue()
    );

    stagingBuffer.Destroy();

    CreateImageView(
        m_Image,
        swapchain.GetColorFormat(),
        VK_IMAGE_ASPECT_COLOR_BIT,
        swapchain.GetComponentMapping(),
        m_ImageView,
        m_Device->GetHandle()
    );
    CreateSampler();
}

VulkanTextureImage::~VulkanTextureImage()
{
}

void VulkanTextureImage::Destroy()
{
    VkDevice deviceHandle = m_Device->GetHandle();
    vkDestroySampler(deviceHandle, m_Sampler, nullptr);
    vkDestroyImageView(deviceHandle, m_ImageView, nullptr);
    vkDestroyImage(deviceHandle, m_Image, nullptr);
    vkFreeMemory(deviceHandle, m_Memory, nullptr);
}

void VulkanTextureImage::CopyBufferToImage(const VulkanBuffer& buffer, VulkanCommandPool& cmdPool)
{
    VulkanCommandBuffer cmdBuf = cmdPool.Allocate();
    cmdBuf.BeginSingle();

    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(m_Width), static_cast<uint32_t>(m_Height), 1};
    cmdBuf.CopyBufferToImage(buffer.GetHandle(), m_Image, &region);

    cmdBuf.End();
    cmdBuf.SubmitOnceTo(*m_Device->GetGraphicsQueue());
    cmdPool.Free(cmdBuf);    
}

void VulkanTextureImage::CreateSampler()
{
    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = m_Device->GetGpuProperties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    VK_CHECK_RESULT(vkCreateSampler(m_Device->GetHandle(), &samplerInfo, nullptr, &m_Sampler));
}

VulkanDepthImage::VulkanDepthImage(
    VulkanDevice* device,
    const VulkanSwapchain& swapchain,
    VulkanCommandPool& cmdPool)
    : m_Image(VK_NULL_HANDLE)
    , m_Device(device)
    , m_Memory(VK_NULL_HANDLE)
    , m_ImageView(VK_NULL_HANDLE)
    , m_DepthFormat(swapchain.GetDepthFormat())
{
    VkExtent2D extent = swapchain.GetExtent();
    CreateImage(
        extent.width,
        extent.height,
        m_DepthFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_Image,
        m_Memory,
        *m_Device
    );

    CreateImageView(
        m_Image,
        m_DepthFormat,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        {},
        m_ImageView,
        m_Device->GetHandle()
    );

    TransitionLayout(
        m_Image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        cmdPool,
        *m_Device->GetGraphicsQueue()
    );
}

VulkanDepthImage::~VulkanDepthImage()
{
}

void VulkanDepthImage::Destroy()
{
    VkDevice deviceHandle = m_Device->GetHandle();
    vkDestroyImageView(deviceHandle, m_ImageView, nullptr);
    vkDestroyImage(deviceHandle, m_Image, nullptr);
    vkFreeMemory(deviceHandle, m_Memory, nullptr);
}

}
