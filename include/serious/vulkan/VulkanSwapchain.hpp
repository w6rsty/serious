#pragma once

#include "serious/vulkan/VulkanDevice.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace serious
{

class VulkanSwapchain final
{
public:
    VulkanSwapchain();
    void SetContext(VkInstance instance, VulkanDevice* device);
    // Create window surface and present queue
    void InitSurface(void* platformWindow);
    void Create(uint32_t* width, uint32_t* height, bool vsync);  
    // Release resources
    void Cleanup();
    void Present(VkSemaphore* renderCompleteSemaphore, uint32_t imageIndex);
    VkResult AcquireNextImage(VkSemaphore presentCompleteSemaphore, uint32_t* imageIndex);

    inline VkExtent2D         GetExtent() const { return m_Extent; }
    inline uint32_t           GetImageCount() const { return m_ImageCount; }
    inline VkFormat           GetColorFormat() const { return m_ColorFormat; }
    inline VkFormat           GetDepthFormat() const { return m_DepthFormat; }
    inline VkComponentMapping GetComponentMapping() const { return m_ComponentMapping; }
    inline VkImageView        GetImageView(uint32_t index) const { return m_ImageViews[index]; }
    inline VkSwapchainKHR     GetHandle() const { return m_Swapchain; }
private:
    VkSwapchainKHR m_Swapchain;
    VkInstance m_Instance;
    VulkanDevice* m_Device;
    VkPhysicalDevice m_Gpu;
    VkSurfaceKHR m_Surface;

    VkExtent2D m_Extent;
    VkPresentModeKHR m_PresentMode;
    uint32_t m_ImageCount;
    VkFormat m_ColorFormat;
    VkColorSpaceKHR m_ColorSpace;
    VkComponentMapping m_ComponentMapping;
    VkFormat m_DepthFormat;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
};

}
