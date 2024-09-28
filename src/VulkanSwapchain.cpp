#include "serious/VulkanSwapchain.hpp"
#include "serious/VulkanContext.hpp"

namespace serious
{

VulkanSwapchain::VulkanSwapchain(uint32_t width, uint32_t height, bool vsync)
    : m_Swapchain(VK_NULL_HANDLE)
    , m_ColorFormat(VK_FORMAT_UNDEFINED)
    , m_ColorSpace(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
{
    Create(width, height, vsync, nullptr);
}

VulkanSwapchain::~VulkanSwapchain()
{
}

void VulkanSwapchain::Destroy()
{
    if (m_Swapchain != VK_NULL_HANDLE) {
        Ref<VulkanDevice> device = VulkanContext::Get().GetDevice();
        VK_CHECK_RESULT(vkQueueWaitIdle(device->GetGraphicsQueue()->GetQueue()));
        VK_CHECK_RESULT(vkQueueWaitIdle(device->GetPresentQueue()->GetQueue()));

        for (VkImageView imageView : m_ImageViews) {
            vkDestroyImageView(device->GetHandle(), imageView, nullptr);
        }
        vkDestroySwapchainKHR(device->GetHandle(), m_Swapchain, nullptr);
    }
}

void VulkanSwapchain::Create(uint32_t width, uint32_t height, bool vsync, VulkanSwapchainRecreateInfo* recreateInfo)
{
    Ref<VulkanDevice> device = VulkanContext::Get().GetDevice();
    VkPhysicalDevice gpu = device->GetGpuHandle();
    VkSurfaceKHR surface = VulkanContext::Get().GetSurfaceHandle();

    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfaceCaps));
    /// See https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
    /// Special value (0xFFFFFFFF, 0xFFFFFFFF) means the extent size is determined by targeting surface
    uint32_t sizeX = (surfaceCaps.currentExtent.width == 0xFFFFFFFF) ? width : surfaceCaps.currentExtent.width;
    uint32_t sizeY = (surfaceCaps.currentExtent.height == 0xFFFFFFFF) ? height : surfaceCaps.currentExtent.height;
    if (surfaceCaps.currentExtent.width == 0 || surfaceCaps.currentExtent.height == 0) {
        return; /// Cancel creation
    }

    uint32_t desiredImageCount = surfaceCaps.minImageCount + 1;
    uint32_t supportedMaxImageCount = surfaceCaps.maxImageCount;
    if ((supportedMaxImageCount > 0) && (desiredImageCount > supportedMaxImageCount)) {
        desiredImageCount = supportedMaxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform;
    if (surfaceCaps.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surfaceCaps.currentTransform;
    }

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

    /// Surface present mode (prefer mailbox)
    uint32_t surfacePresentModeCount = 0;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &surfacePresentModeCount, nullptr));
    std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &surfacePresentModeCount, surfacePresentModes.data()));
    VkPresentModeKHR swapchainPresentMode;
    swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR; /// Default support for vsync
    if (!vsync) {
        for (const VkPresentModeKHR& presentMode : surfacePresentModes) {
            /// Better one
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                swapchainPresentMode = presentMode;
                break;
            }
            /// Fastest, but may cause tearing
            if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                swapchainPresentMode = presentMode;
                break;
            }
        }
    }

    /// Image format and color space
    uint32_t surfaceFormatCount = 0;    
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surfaceFormatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &surfaceFormatCount, surfaceFormats.data()));
    m_ColorFormat = surfaceFormats[0].format;
    m_ColorSpace = surfaceFormats[0].colorSpace;
    if ((surfaceFormatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
        m_ColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
        m_ColorSpace = surfaceFormats[0].colorSpace;
    } else {
        for (const VkSurfaceFormatKHR& format : surfaceFormats) {
            if ((format.format == VK_FORMAT_R8G8B8A8_UNORM) || (format.format == VK_FORMAT_B8G8R8A8_UNORM)) {
                m_ColorFormat = format.format;
                m_ColorSpace = format.colorSpace;
                break;
            }
        }
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface;
    swapchainInfo.presentMode = swapchainPresentMode;
    swapchainInfo.imageFormat = m_ColorFormat;
    swapchainInfo.imageColorSpace = m_ColorSpace;
    swapchainInfo.imageExtent.width = sizeX;
    swapchainInfo.imageExtent.height = sizeY;
    swapchainInfo.minImageCount = desiredImageCount;
    swapchainInfo.preTransform = preTransform;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.compositeAlpha = compositeAlpha;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
    if (recreateInfo != nullptr) {
        swapchainInfo.oldSwapchain = recreateInfo->oldSwapchain;
    }
    VK_CHECK_RESULT(vkCreateSwapchainKHR(VulkanContext::Get().GetDevice()->GetHandle(), &swapchainInfo, nullptr, &m_Swapchain));
    Info("create swapchain with {} {} {} num images {} {}x{}", VulkanPresentModeString(swapchainPresentMode), VulkanFormatString(m_ColorFormat), VulkanColorSpaceString(m_ColorSpace), desiredImageCount, sizeX, sizeY);

    if (recreateInfo != nullptr) {
        vkQueueWaitIdle(device->GetGraphicsQueue()->GetQueue());
        vkQueueWaitIdle(device->GetPresentQueue()->GetQueue());
        vkDestroySwapchainKHR(device->GetHandle(), recreateInfo->oldSwapchain, nullptr);

        for (VkImageView imageView : m_ImageViews) {
            vkDestroyImageView(device->GetHandle(), imageView, nullptr);
        }
        m_ImageViews.clear();
    }

    /// Get images
    uint32_t numSwapchainImages = 0;;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device->GetHandle(), m_Swapchain, &numSwapchainImages, nullptr));
    m_Images.resize(numSwapchainImages);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device->GetHandle(), m_Swapchain, &numSwapchainImages, m_Images.data()));

    /// Create image views
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = m_ColorFormat;
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    m_ImageViews.resize(m_Images.size());
    for (size_t i = 0; i < m_Images.size(); i++) {
        imageViewInfo.image = m_Images[i];
        VK_CHECK_RESULT(vkCreateImageView(device->GetHandle(), &imageViewInfo, nullptr, &m_ImageViews[i]));
    }

    /// Reset in fight id
    m_PresentId = 0;
}

}
