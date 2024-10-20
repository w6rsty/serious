#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanPass.hpp"
#include "serious/vulkan/VulkanWindow.hpp"
#include "serious/VulkanUtils.hpp"

#include <Tracy.hpp>

namespace serious
{

VulkanSwapchain::VulkanSwapchain(VulkanDevice* device, VulkanWindow* window)
    : m_Swapchain(VK_NULL_HANDLE)
    , m_Device(device)
    , m_Window(window)
    , m_WindowSpec({})
    , m_ColorFormat(VK_FORMAT_UNDEFINED)
    , m_ColorSpace(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    , m_ComponentMapping({VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A})
    , m_CurrentImage(UINT32_MAX)
{
}

VulkanSwapchain::~VulkanSwapchain()
{
}

void VulkanSwapchain::Destroy()
{
    m_Device->GetGraphicsQueue()->WaitIdle();
    vkDestroySwapchainKHR(m_Device->GetHandle(), m_Swapchain, nullptr);
}

void VulkanSwapchain::Create()
{
    m_WindowSpec = m_Window->GetWindowSpec();
    VkSurfaceKHR surface = m_Window->GetSurfaceHandle();

    VkPhysicalDevice gpu = m_Device->GetGpuHandle();

    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfaceCaps));
    if (surfaceCaps.currentExtent.width == 0 || surfaceCaps.currentExtent.height == 0) {
        return; /// Cancel creation
    }
    /// See https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
    /// Special value (0xFFFFFFFF, 0xFFFFFFFF) means the extent size is determined by targeting surface
    uint32_t sizeX = (surfaceCaps.currentExtent.width == 0xFFFFFFFF) ? m_WindowSpec.width : surfaceCaps.currentExtent.width;
    uint32_t sizeY = (surfaceCaps.currentExtent.height == 0xFFFFFFFF) ? m_WindowSpec.height : surfaceCaps.currentExtent.height;

    // Do triple-buffering when possible
    uint32_t desiredImageCount = std::max(surfaceCaps.minImageCount, 3u);
    if (desiredImageCount > surfaceCaps.maxImageCount) {
        desiredImageCount = std::min(desiredImageCount, surfaceCaps.maxImageCount);
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
    if (!m_WindowSpec.vsync) {
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
    if (m_ColorFormat == VK_FORMAT_B8G8R8A8_UNORM) {
        std::swap(m_ComponentMapping.r, m_ComponentMapping.b);
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
    swapchainInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.compositeAlpha = compositeAlpha;
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK_RESULT(vkCreateSwapchainKHR(m_Device->GetHandle(), &swapchainInfo, nullptr, &m_Swapchain));
    Info("create swapchain with {} {} {} num images {} {}x{}", VulkanPresentModeString(swapchainPresentMode), VulkanFormatString(m_ColorFormat), VulkanColorSpaceString(m_ColorSpace), desiredImageCount, sizeX, sizeY);
}



void VulkanSwapchain::Present(VulkanSemaphore* outSemaphore)
{
    VkSemaphore samephore = outSemaphore->GetHandle();

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &m_Swapchain;
    presentInfo.pImageIndices = &m_CurrentImage;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &samephore;
    VK_CHECK_RESULT(vkQueuePresentKHR(m_Device->GetPresentQueue()->GetHandle(), &presentInfo));
}

void VulkanSwapchain::AcquireImage(VulkanSemaphore* outSemaphore, uint32_t& imageIndex)
{
    ZoneScopedN("Acquire Image");
    uint32_t index = UINT32_MAX;
    VK_CHECK_RESULT(vkAcquireNextImageKHR(m_Device->GetHandle(), m_Swapchain, UINT64_MAX, outSemaphore->GetHandle(), VK_NULL_HANDLE, &index));
    imageIndex = index;
    m_CurrentImage = index;
}

}
