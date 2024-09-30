#include "serious/VulkanSwapchain.hpp"
#include "serious/VulkanContext.hpp"
#include "serious/VulkanPass.hpp"

namespace serious
{

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, uint32_t width, uint32_t height, bool vsync)
    : m_Swapchain(VK_NULL_HANDLE)
    , m_Device(device)
    , m_CurrentImageIndex(0)
    , m_Extent({})
    , m_ColorFormat(VK_FORMAT_UNDEFINED)
    , m_ColorSpace(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    , m_EnableVSync(vsync)
    , m_Images({})
    , m_ImageViews({})
    , m_Framebuffers({})
    , m_GetWindowSizeFunc(nullptr)
{
    Create(width, height, vsync, nullptr);
}

VulkanSwapchain::~VulkanSwapchain()
{
}

void VulkanSwapchain::Destroy()
{
    if (m_Swapchain != VK_NULL_HANDLE) {
        VK_CHECK_RESULT(vkQueueWaitIdle(m_Device->GetGraphicsQueue()->GetHandle()));
        VK_CHECK_RESULT(vkQueueWaitIdle(m_Device->GetPresentQueue()->GetHandle()));

        VkDevice device = m_Device->GetHandle();
        for (VkImageView imageView : m_ImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
    }
}

void VulkanSwapchain::Create(uint32_t width, uint32_t height, bool vsync, VulkanSwapchainRecreateInfo* recreateInfo)
{
    VkPhysicalDevice gpu = m_Device->GetGpuHandle();
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
    m_Extent.width = width;
    m_Extent.height = height;

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
    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_Device->GetHandle(), &swapchainInfo, nullptr, &m_Swapchain));
    Info("create swapchain with {} {} {} num images {} {}x{}", VulkanPresentModeString(swapchainPresentMode), VulkanFormatString(m_ColorFormat), VulkanColorSpaceString(m_ColorSpace), desiredImageCount, sizeX, sizeY);

    if (recreateInfo != nullptr) {
        vkQueueWaitIdle(m_Device->GetGraphicsQueue()->GetHandle());
        vkQueueWaitIdle(m_Device->GetPresentQueue()->GetHandle());
        vkDestroySwapchainKHR(m_Device->GetHandle(), recreateInfo->oldSwapchain, nullptr);

        for (VkImageView imageView : m_ImageViews) {
            vkDestroyImageView(m_Device->GetHandle(), imageView, nullptr);
        }
        m_ImageViews.clear();
    }

    /// Get images
    uint32_t numSwapchainImages = 0;;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_Device->GetHandle(), m_Swapchain, &numSwapchainImages, nullptr));
    m_Images.resize(numSwapchainImages);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_Device->GetHandle(), m_Swapchain, &numSwapchainImages, m_Images.data()));

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
        VK_CHECK_RESULT(vkCreateImageView(m_Device->GetHandle(), &imageViewInfo, nullptr, &m_ImageViews[i]));
    }
}

void VulkanSwapchain::OnResize()
{
    m_Extent = m_GetWindowSizeFunc();
    VulkanSwapchainRecreateInfo recreateInfo;
    recreateInfo.oldSwapchain = m_Swapchain;
    Create(m_Extent.width, m_Extent.height, m_EnableVSync, &recreateInfo);
}

void VulkanSwapchain::CreateFramebuffers(VulkanRenderPass* pass)
{
    m_Framebuffers.resize(m_Images.size());
    
    VkDevice device = m_Device->GetHandle();

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = pass->GetHandle();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = m_Extent.width;
    framebufferInfo.height = m_Extent.height;
    framebufferInfo.layers = 1;
    for (size_t i = 0; i < m_Framebuffers.size(); ++i) {
        framebufferInfo.pAttachments = &m_ImageViews[i];
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffers[i]));
    }
}

void VulkanSwapchain::DestroyFrameBuffers()
{
    VkDevice device = m_Device->GetHandle();
    for (VkFramebuffer& framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
}

void VulkanSwapchain::Present(VulkanSemaphore* outSemaphore)
{
    VkSemaphore samephore = outSemaphore->GetHandle();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &m_CurrentImageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &samephore;
    VK_CHECK_RESULT(vkQueuePresentKHR(m_Device->GetPresentQueue()->GetHandle(), &presentInfo));
}

uint32_t VulkanSwapchain::AcquireNextImage(VulkanSemaphore* outSemaphore)
{
    VkDevice device = m_Device->GetHandle();
    /// Signal to m_PresentComplete 
    VkResult result = vkAcquireNextImageKHR(device, m_Swapchain, UINT64_MAX, outSemaphore->GetHandle(), VK_NULL_HANDLE, &m_CurrentImageIndex);
    if (result != VK_SUCCESS) {
        if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
            OnResize();
            /// Recursive may goes wrong
            VK_CHECK_RESULT(vkAcquireNextImageKHR(device, m_Swapchain, UINT64_MAX, outSemaphore->GetHandle(), VK_NULL_HANDLE, &m_CurrentImageIndex));
        }
    }
    return m_CurrentImageIndex;
}



}
