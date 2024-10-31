#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/VulkanUtils.hpp"

#include <Tracy.hpp>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace serious
{

VulkanSwapchain::VulkanSwapchain()
    : m_Swapchain(VK_NULL_HANDLE)
    , m_Instance(VK_NULL_HANDLE)
    , m_Device(nullptr)
    , m_Gpu(VK_NULL_HANDLE)
    , m_Surface(VK_NULL_HANDLE)
    , m_Extent({0, 0})
    , m_PresentMode(VK_PRESENT_MODE_FIFO_KHR)
    , m_ImageCount(0)
    , m_ColorFormat(VK_FORMAT_UNDEFINED)
    , m_ColorSpace(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    , m_ComponentMapping({VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A})
    , m_DepthFormat(VK_FORMAT_D32_SFLOAT)
    , m_Images({})
    , m_ImageViews({})
{
}

void VulkanSwapchain::SetContext(VkInstance instance, VulkanDevice* device)
{
    m_Device = device;
    m_Instance = instance;
    m_Gpu = m_Device->GetGpuHandle(); // Assuming no GPU switch
}

void VulkanSwapchain::InitSurface(void* platformWindow)
{
    SDL_Vulkan_CreateSurface(static_cast<SDL_Window*>(platformWindow), m_Instance, nullptr, &m_Surface);
    m_Device->SetPresentQueue(m_Surface);
}

void VulkanSwapchain::Cleanup()
{
    VkDevice device = m_Device->GetHandle();
    for (VkImageView& imageView : m_ImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    if (m_Surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    }
    vkDestroySwapchainKHR(device, m_Swapchain, nullptr);
    
    m_Swapchain = VK_NULL_HANDLE;
}

void VulkanSwapchain::Create(uint32_t* width, uint32_t* height, bool vsync)
{
    VkSwapchainKHR oldSwapchain = m_Swapchain;
    VkDevice device = m_Device->GetHandle();

    VkSurfaceCapabilitiesKHR surfaceCaps {};
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Gpu, m_Surface, &surfaceCaps));
    /// See https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
    /// Special value 0xFFFFFFFF means the extent size is determined by targeting surface
    if (surfaceCaps.currentExtent.width == 0 || surfaceCaps.currentExtent.height == 0) {
        return;
    }
    if (surfaceCaps.currentExtent.width == (uint32_t)-1) {
        m_Extent.width = *width;
        m_Extent.height = *height;
    } else {
        m_Extent = surfaceCaps.currentExtent;
        *width = surfaceCaps.currentExtent.width;
        *height = surfaceCaps.currentExtent.height;
    }

        /// Surface present mode (prefer mailbox)
    uint32_t surfacePresentModeCount = 0;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_Gpu, m_Surface, &surfacePresentModeCount, nullptr));
    std::vector<VkPresentModeKHR> surfacePresentModes(surfacePresentModeCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(m_Gpu, m_Surface, &surfacePresentModeCount, surfacePresentModes.data()));
    m_PresentMode = VK_PRESENT_MODE_FIFO_KHR; /// Default support for vsync
    if (!vsync) {
        for (const VkPresentModeKHR& presentMode : surfacePresentModes) {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // Primary
                m_PresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
            /// Fastest, but may cause tearing
            if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) { // Secondary
                m_PresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            }
        }
    }

    // Do triple-buffering when possible
    m_ImageCount = std::max(surfaceCaps.minImageCount, 3u);
    if (m_ImageCount > surfaceCaps.maxImageCount) {
        m_ImageCount = std::min(m_ImageCount, surfaceCaps.maxImageCount);
    }

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    if (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }

        /// Image format and color space
    uint32_t surfaceFormatCount = 0;    
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_Gpu, m_Surface, &surfaceFormatCount, nullptr));
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(m_Gpu, m_Surface, &surfaceFormatCount, surfaceFormats.data()));
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
    if (m_ColorFormat == VK_FORMAT_B8G8R8A8_UNORM) {
        std::swap(m_ComponentMapping.r, m_ComponentMapping.b);
    }

    VkSwapchainCreateInfoKHR swapchainInfo {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = m_Surface;
    swapchainInfo.imageFormat = m_ColorFormat;
    swapchainInfo.imageColorSpace = m_ColorSpace;
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.imageExtent = m_Extent;
    swapchainInfo.presentMode = m_PresentMode;
    swapchainInfo.minImageCount = m_ImageCount;
    swapchainInfo.compositeAlpha = compositeAlpha;
    swapchainInfo.oldSwapchain = oldSwapchain;
    VK_CHECK_RESULT(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &m_Swapchain));

    VKInfo(
        "Create swapchain {} | {} | {} | num images {}:{}x{}",
        VulkanPresentModeString(m_PresentMode),
        VulkanFormatString(m_ColorFormat),
        VulkanColorSpaceString(m_ColorSpace),
        m_ImageCount,
        m_Extent.width,
        m_Extent.height
    );

    if (oldSwapchain != VK_NULL_HANDLE) {
        for (VkImageView& imageView : m_ImageViews) {
            vkDestroyImageView(device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
    }

    m_Images.resize(m_ImageCount, VK_NULL_HANDLE);
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(device, m_Swapchain, &m_ImageCount, m_Images.data()));
    m_ImageViews.resize(m_ImageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < m_ImageCount; ++i) {
        m_ImageViews[i] = m_Device->CreateImageView(m_Images[i], m_ColorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

VkResult VulkanSwapchain::Present(VkSemaphore* renderCompleteSemaphore, uint32_t imageIndex)
{
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = renderCompleteSemaphore;
    return vkQueuePresentKHR(m_Device->GetPresentQueue()->GetHandle(), &presentInfo);
}

VkResult VulkanSwapchain::AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex)
{
    return vkAcquireNextImageKHR(m_Device->GetHandle(), m_Swapchain, UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, imageIndex);
}

}
