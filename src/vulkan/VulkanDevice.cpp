#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanCommand.hpp"

#include <cassert>

#include <stb_image.h>

namespace serious
{

VulkanQueue::VulkanQueue(VulkanDevice* device, uint32_t familyIndex)
    : m_Queue(VK_NULL_HANDLE)
    , m_QueueIndex(0)
    , m_FamilyIndex(familyIndex)
    , m_Device(device)
{
    vkGetDeviceQueue(m_Device->GetHandle(), m_FamilyIndex, m_QueueIndex, &m_Queue);
}

VulkanQueue::~VulkanQueue()
{
}

void VulkanQueue::Submit(const VkSubmitInfo& submitInfo, VkFence fence)
{
    VK_CHECK_RESULT(vkQueueSubmit(m_Queue, 1, &submitInfo, fence));
}

void VulkanQueue::WaitIdle()
{
    VK_CHECK_RESULT(vkQueueWaitIdle(m_Queue));
}

VulkanDevice::VulkanDevice(VkInstance instance)
    : m_Device(VK_NULL_HANDLE)
    , m_Gpu(VK_NULL_HANDLE)
    , m_GpuProps({})
    , m_GpuMemoryProps({})
    , m_DeviceLocalMemorySupport(false)
    , m_GraphicsQueue(nullptr)
    , m_ComputeQueue(nullptr)
    , m_TransferQueue(nullptr)
    , m_PresentQueue(nullptr)
    , m_DescriptorSetLayout(VK_NULL_HANDLE)
    , m_DescriptorPool(VK_NULL_HANDLE)
{
    SelectGpu(instance);
    vkGetPhysicalDeviceMemoryProperties(m_Gpu, &m_GpuMemoryProps);
    for (uint32_t i = 0; i < m_GpuMemoryProps.memoryTypeCount; ++i) {
        if (m_GpuMemoryProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            m_DeviceLocalMemorySupport = true;
            VKInfo("-- Device local memory supported");
            break;
        }
    }

    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_Gpu, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedDeviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(m_Gpu, nullptr, &deviceExtensionCount, supportedDeviceExtensions.data());
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME  
    };
    if (!validateExtension(deviceExtensions, supportedDeviceExtensions)) {
        VKFatal("Required device extensions not found");
    }

    /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
    /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateInfo.html
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &queueFamilyCount, queueFamilyProperties.data());
    VKInfo("-- Max anisotropy: {}", m_GpuProps.limits.maxSamplerAnisotropy);
    VKInfo("-- Found {} available queue(s)", queueFamilyCount);
    
    /// Queues create infos(Reference from Unreal Engine VulkanRHI)
    std::vector<VkDeviceQueueCreateInfo> queueFamilyInfos;
    uint32_t numPriorities = 0;
    int32_t graphicsQueueFamilyIndex = -1;
    int32_t computeQueueFamilyIndex  = -1;
    int32_t transferQueueFamilyIndex = -1;

    constexpr VkFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    VkFlags dispatchedQueues = 0;

    for (int32_t familyIndex = 0; familyIndex < static_cast<int32_t>(queueFamilyCount); ++familyIndex) {
        const VkQueueFamilyProperties& currProps = queueFamilyProperties[static_cast<size_t>(familyIndex)];

        bool isValidQueue = false;
        if ((currProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (graphicsQueueFamilyIndex == -1)) {
            graphicsQueueFamilyIndex = familyIndex;
            isValidQueue = true;
            dispatchedQueues |= VK_QUEUE_GRAPHICS_BIT;
        } else if ((currProps.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            (computeQueueFamilyIndex == -1) &&
            (graphicsQueueFamilyIndex != familyIndex)) {
            computeQueueFamilyIndex = familyIndex;
            isValidQueue = true;
            dispatchedQueues |= VK_QUEUE_COMPUTE_BIT;
        } else if ((currProps.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            (transferQueueFamilyIndex == -1) &&
            (graphicsQueueFamilyIndex != familyIndex) &&
            (computeQueueFamilyIndex  != familyIndex)) {
            transferQueueFamilyIndex = familyIndex;
            isValidQueue = true;
            dispatchedQueues |= VK_QUEUE_TRANSFER_BIT;
        }
        if (!isValidQueue) {
            VKInfo("Skipped queue {}({})", familyIndex, currProps.queueCount);
            continue;
        }

        size_t queueIndex = queueFamilyInfos.size();
        queueFamilyInfos.emplace_back();
        VkDeviceQueueCreateInfo& currQueue = queueFamilyInfos[queueIndex];
        currQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        currQueue.queueCount = currProps.queueCount;
        currQueue.queueFamilyIndex = static_cast<uint32_t>(familyIndex);
        numPriorities += currProps.queueCount;

        if ((dispatchedQueues & requiredFlags) == requiredFlags) {
            break;
        }
    }

    std::vector<float> queuePriorities(numPriorities);
    float* currPriority = queuePriorities.data();
    for (uint32_t infoIndex = 0; infoIndex < queueFamilyInfos.size(); ++infoIndex) {
        VkDeviceQueueCreateInfo& currQueue = queueFamilyInfos[infoIndex];
        currQueue.pQueuePriorities = currPriority;

        const VkQueueFamilyProperties& currProps = queueFamilyProperties[currQueue.queueFamilyIndex];
        for (uint32_t queueIndex = 0; queueIndex < currProps.queueCount; ++queueIndex) {
            *currPriority++ = 1.0f;
        }
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE; // enable anisotropy manually

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.queueCreateInfoCount = queueFamilyInfos.size();
    deviceInfo.pQueueCreateInfos = queueFamilyInfos.data();
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    VK_CHECK_RESULT(vkCreateDevice(m_Gpu, &deviceInfo, nullptr, &m_Device));

    /// Create queues https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetDeviceQueue.html
    m_GraphicsQueue = CreateRef<VulkanQueue>(this, graphicsQueueFamilyIndex);
    VKInfo("Using queue {} for graphics", graphicsQueueFamilyIndex);    
    if (computeQueueFamilyIndex == -1) {
        computeQueueFamilyIndex = graphicsQueueFamilyIndex;
    }
    m_ComputeQueue = CreateRef<VulkanQueue>(this, computeQueueFamilyIndex);
    VKInfo("Using queue {} for compute", computeQueueFamilyIndex);
    if (transferQueueFamilyIndex == -1) {
        transferQueueFamilyIndex = computeQueueFamilyIndex;
    }
    m_TransferQueue = CreateRef<VulkanQueue>(this, transferQueueFamilyIndex);
    VKInfo("Using queue {} for transfer", transferQueueFamilyIndex);
}

VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::Destroy()
{
    vkDestroyDevice(m_Device, nullptr);
}

void VulkanDevice::SetPresentQueue(VkSurfaceKHR surface)
{
    const auto isSupportPresent = [surface](VkPhysicalDevice gpu, Ref<VulkanQueue> queue) {
        VkBool32 result = VK_FALSE;
        uint32_t familyIndex = queue->GetFamilyIndex();
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, familyIndex, surface, &result));
        return result == VK_TRUE;
    };
    if (!isSupportPresent(m_Gpu, m_GraphicsQueue)) {
        VKFatal("failed to find a queue family that supports presentation");
    }
    if (isSupportPresent(m_Gpu, m_ComputeQueue)) {
        m_PresentQueue = m_ComputeQueue;
    } else {
        m_PresentQueue = m_GraphicsQueue;
    }
    VKInfo("Using queue {} for presentation", m_PresentQueue->GetFamilyIndex());
}

void VulkanDevice::WaitIdle()
{
    VK_CHECK_RESULT(vkDeviceWaitIdle(m_Device));
}

VulkanShaderModule VulkanDevice::CreateShaderModule(const std::string& path, VkShaderStageFlagBits flag)
{
    VulkanShaderModule shaderModule {};
    shaderModule.stage = flag;
    std::string source = ReadFile(path);
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = source.size();
    shaderModuleInfo.pCode = (uint32_t*)source.data();
    VK_CHECK_RESULT(vkCreateShaderModule(m_Device, &shaderModuleInfo, nullptr, &shaderModule.handle));
    return shaderModule;
}

void VulkanDevice::DestroyShaderModule(VulkanShaderModule& shaderModule)
{
    vkDestroyShaderModule(m_Device, shaderModule.handle, nullptr);
}

VulkanFence VulkanDevice::CreateFence(VkFenceCreateFlags flags)
{
    return VulkanFence(m_Device, flags);
}

void VulkanDevice::DestroyFence(VulkanFence& fence)
{
    vkDestroyFence(m_Device, fence.m_Fence, nullptr);
}

VkSemaphore VulkanDevice::CreateSemaphore()
{
    VkSemaphore semaphore;
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &semaphore));
    return semaphore;
}

VulkanImage VulkanDevice::CreateImage(
    uint32_t width, uint32_t height,
    VkFormat format,
    VkImageTiling imageTiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    VulkanImage image;

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
    VK_CHECK_RESULT(vkCreateImage(m_Device, &imageInfo, nullptr, &image.image));

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device, image.image, &memRequirements);

    VkMemoryAllocateInfo allocateInfo {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = FindMemoryTypeIdx(memRequirements.memoryTypeBits, properties);
    VK_CHECK_RESULT(vkAllocateMemory(m_Device, &allocateInfo, nullptr, &image.memory));
    vkBindImageMemory(m_Device, image.image, image.memory, 0);

    return image;
}

void VulkanDevice::DestroyImage(VulkanImage& image)
{
    vkDestroyImage(m_Device, image.image, nullptr);
    vkFreeMemory(m_Device, image.memory, nullptr);
}

VkImageView VulkanDevice::CreateImageView(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspectFlags,
    VkComponentMapping mapping)
{
    VkImageView imageView;
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
    VK_CHECK_RESULT(vkCreateImageView(m_Device, &viewInfo, nullptr, &imageView));
    return imageView;
}

VkFramebuffer VulkanDevice::CreateFramebuffer(
    const VkExtent2D& extent,
    VkRenderPass renderPass,
    const std::vector<VkImageView>& attachments)
{
    VkFramebuffer framebuffer;
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.width = extent.width;
    framebufferInfo.height = extent.height;
    framebufferInfo.layers = 1;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    VK_CHECK_RESULT(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &framebuffer));
    return framebuffer;
}

VulkanCommandPool VulkanDevice::CreateCommandPool(const VulkanQueue& queue)
{
    VulkanCommandPool cmdPool;
    cmdPool.m_Device = m_Device;
    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queue.GetFamilyIndex();
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VK_CHECK_RESULT(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &cmdPool.m_CmdPool));
    return cmdPool;
}

void VulkanDevice::CreateBuffer(
    VulkanBuffer& buffer,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo bufferInfo {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK_RESULT(vkCreateBuffer(m_Device, &bufferInfo, nullptr, &buffer.buffer));

    VkMemoryRequirements memoryRequirements {};
    vkGetBufferMemoryRequirements(m_Device, buffer.buffer, &memoryRequirements);
    
    VkMemoryAllocateInfo allocateInfo {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = FindMemoryTypeIdx(memoryRequirements.memoryTypeBits, properties);

    VK_CHECK_RESULT(vkAllocateMemory(m_Device, &allocateInfo, nullptr, &buffer.memory));
    vkBindBufferMemory(m_Device, buffer.buffer, buffer.memory, 0);
}

void VulkanDevice::CopyToBuffer(VulkanBuffer& buffer, const void* data, VkDeviceSize size)
{
    assert(buffer.mapped);
    memcpy(buffer.mapped, data, size);
}

void VulkanDevice::DestroyBuffer(VulkanBuffer& buffer)
{
    vkDestroyBuffer(m_Device, buffer.buffer, nullptr);
    vkFreeMemory(m_Device, buffer.memory, nullptr);
}

void VulkanDevice::CopyBuffer(
    VkBuffer src,
    VkBuffer dst,
    VkDeviceSize size,
    VkDeviceSize offset,
    VulkanCommandBuffer& tsfCmd)
{
    tsfCmd.BeginSingle();
    tsfCmd.CopyBuffer(src, dst, size, offset);
    tsfCmd.End();
    
    VulkanFence copyFence = CreateFence();
    tsfCmd.SubmitOnceTo(*m_TransferQueue, copyFence.m_Fence);
    copyFence.Wait();
    DestroyFence(copyFence);
}

void VulkanDevice::CreateDeviceBuffer(
    VulkanBuffer& buffer,
    VkDeviceSize size,
    void* data,
    VkBufferUsageFlags usage,
    VulkanCommandBuffer& tsfCmd)
{
    if (m_DeviceLocalMemorySupport) {
        VulkanBuffer stagingBuffer;
        CreateBuffer(stagingBuffer, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        MapBuffer(stagingBuffer, size, 0);
        CopyToBuffer(stagingBuffer, data, size);
        UnmapBuffer(stagingBuffer);
        CreateBuffer(buffer, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        CopyBuffer(stagingBuffer.buffer, buffer.buffer, size, 0, tsfCmd);
        DestroyBuffer(stagingBuffer);
    } else {
        VKWarn("Device local memory not supported, using host visible memory");
        CreateBuffer(buffer, size, usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

void VulkanDevice::MapBuffer(VulkanBuffer& buffer, VkDeviceSize size, VkDeviceSize offset)
{
    VK_CHECK_RESULT(vkMapMemory(m_Device, buffer.memory, offset, size, 0, &buffer.mapped));
}

void VulkanDevice::UnmapBuffer(VulkanBuffer& buffer)
{
    vkUnmapMemory(m_Device, buffer.memory);
}

void VulkanDevice::TransitionImageLayout(
    VkImage image,
    VkImageLayout srcLayout,
    VkImageLayout dstLayout,
    VkImageAspectFlags aspectFlags,
    VulkanCommandBuffer& cmd)
{
    cmd.BeginSingle();

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
        VKError("Unsupported layout transition");
    }
    cmd.PipelineImageBarrier(srcStage, dstStage, &barrier);
    cmd.End();

    VulkanFence transitionFence = CreateFence();
    cmd.SubmitOnceTo(*m_GraphicsQueue, transitionFence.m_Fence);
    transitionFence.Wait();
    DestroyFence(transitionFence);
}

void VulkanDevice::CreateTextureImage(
    VulkanTexture& texture,
    const std::string& path,
    VkFormat format,
    VkComponentMapping mapping,
    VulkanCommandBuffer& gfxCmd)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        VKWarn("Failed to load image {}", path);
        width = 1;
        height = 1;
        static constexpr uint32_t errorColor = 0xFF00FFFF;
        pixels = (stbi_uc*)(&errorColor);
    }
    texture.width = static_cast<uint32_t>(width);
    texture.height = static_cast<uint32_t>(height);
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width * height * 4);
    VulkanBuffer stagingBuffer;
    CreateBuffer(stagingBuffer, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    MapBuffer(stagingBuffer, imageSize, 0);
    CopyToBuffer(stagingBuffer, pixels, imageSize);
    UnmapBuffer(stagingBuffer);
    stbi_image_free(pixels);

    texture.image = CreateImage(
        texture.width, texture.height,
        format, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        m_DeviceLocalMemorySupport ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    );
    TransitionImageLayout(texture.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, gfxCmd);
    gfxCmd.BeginSingle();
    VkBufferImageCopy region {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {texture.width, texture.height, 1};
    gfxCmd.CopyBufferToImage(stagingBuffer.buffer, texture.image.image, &region);
    gfxCmd.End();
    VulkanFence copyFence = CreateFence();
    gfxCmd.SubmitOnceTo(*m_TransferQueue, copyFence.m_Fence);
    copyFence.Wait();
    TransitionImageLayout(texture.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, gfxCmd);

    DestroyBuffer(stagingBuffer);

    texture.imageView = CreateImageView(texture.image.image, format, VK_IMAGE_ASPECT_COLOR_BIT, mapping);

    VkSamplerCreateInfo samplerInfo {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = m_GpuProps.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    VK_CHECK_RESULT(vkCreateSampler(m_Device, &samplerInfo, nullptr, &texture.sampler));
}

void VulkanDevice::CreateDepthImage(VulkanTexture& texture, const VkExtent2D& extent, VulkanCommandBuffer& gfxCmd)
{
    texture.image = CreateImage(
        extent.width, extent.height,
        VK_FORMAT_D32_SFLOAT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        m_DeviceLocalMemorySupport ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT : VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    );
    TransitionImageLayout(texture.image.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, gfxCmd);
    texture.imageView = CreateImageView(texture.image.image, VK_FORMAT_D32_SFLOAT, VK_IMAGE_ASPECT_DEPTH_BIT);
}


void VulkanDevice::DestroyTextureImage(VulkanTexture& texture)
{
    if (texture.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_Device, texture.sampler, nullptr);
    }
    vkDestroyImageView(m_Device, texture.imageView, nullptr);
    DestroyImage(texture.image);
}

void VulkanDevice::DestroyCommandPool(VulkanCommandPool& cmdPool)
{
    vkDestroyCommandPool(m_Device, cmdPool.m_CmdPool, nullptr);
}

void VulkanDevice::SetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo {};
    descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descriptorSetLayoutInfo.pBindings = bindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_Device, &descriptorSetLayoutInfo, nullptr, &m_DescriptorSetLayout));
}

void VulkanDevice::SetDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets)
{
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = maxSets;
    VK_CHECK_RESULT(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &m_DescriptorPool));
}

void VulkanDevice::AllocateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets)
{
    assert(m_DescriptorPool != VK_NULL_HANDLE);
    assert(m_DescriptorSetLayout != VK_NULL_HANDLE);

    std::vector<VkDescriptorSetLayout> layouts(descriptorSets.size(), m_DescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSets.size());
    allocInfo.pSetLayouts = layouts.data();
    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_Device, &allocInfo, descriptorSets.data()));
}

void VulkanDevice::DestroyDescriptorResources()
{
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
    }
    if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_Device, m_DescriptorSetLayout, nullptr);
    }
}

void VulkanDevice::SelectGpu(VkInstance instance)
{
    uint32_t physicalDeviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0) {
        VKFatal("No device supporting Vulkan found");
    } else {
        VKInfo("Found {} physical device(s)", physicalDeviceCount);
    }
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    int highestScore = -1;
    VkPhysicalDevice bestGpu = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties bestGpuProps = {};

    for (const auto& gpu : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpu, &properties);

        int score = 0;
        // Prefer discrete GPU
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            score += 1000;
        }
        if (score > highestScore) {
            highestScore = score;
            bestGpu = gpu;
            bestGpuProps = properties;
        }
    }

    m_Gpu = bestGpu;
    m_GpuProps = bestGpuProps;
    VKInfo("Using device: {} score: {}", m_GpuProps.deviceName, highestScore);
}

uint32_t VulkanDevice::FindMemoryTypeIdx(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags)
{
    for (uint32_t i = 0; i < m_GpuMemoryProps.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) && (m_GpuMemoryProps.memoryTypes[i].propertyFlags & propertyFlags)) {
            return i;
        }
    }
    VKWarn("Failed to find asked memory type");
    return UINT32_MAX;   
}

}
