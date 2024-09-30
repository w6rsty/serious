#pragma once

#include "serious/VulkanSyncs.hpp"

#include <vulkan/vulkan.h>

#include <vector>
#include <functional>

namespace serious
{

class VulkanDevice;
class VulkanCommandPool;
class VulkanCommandBuffer;
class VulkanRenderPass;

using GetWindowSizeFunc = std::function<VkExtent2D()>;

struct VulkanSwapchainRecreateInfo
{
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
};

class VulkanSwapchain final
{
public:
    VulkanSwapchain(VulkanDevice* device, uint32_t width, uint32_t height, bool vsync);
    ~VulkanSwapchain();

    void Create(uint32_t width, uint32_t height, bool vsync, VulkanSwapchainRecreateInfo* recreateInfo);
    void Destroy();
    void Present(VulkanSemaphore* outSemaphore);
    uint32_t AcquireNextImage(VulkanSemaphore* outSemaphore);
    void OnResize();
    void CreateFramebuffers(VulkanRenderPass* pass);
    void DestroyFrameBuffers();

    inline void SetGetSize(GetWindowSizeFunc func) { m_GetWindowSizeFunc = func; }
    inline VkExtent2D GetExtent() const { return m_Extent; }
    inline VkFormat GetColorFormat() const { return m_ColorFormat; }
    inline VkImage GetImage(size_t index) const { return m_Images[index]; }
    inline VkImageView GetImageView(size_t index) const { return m_ImageViews[index]; }
    inline VkFramebuffer GetFramebuffer(size_t index) const { return m_Framebuffers[index]; }
    inline VkSwapchainKHR GetHandle() const { return m_Swapchain; }
private:
    VkSwapchainKHR m_Swapchain;
    VulkanDevice* m_Device;

    uint32_t m_CurrentImageIndex;

    VkExtent2D m_Extent;
    VkFormat m_ColorFormat;
    VkColorSpaceKHR m_ColorSpace;
    bool m_EnableVSync;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;

    GetWindowSizeFunc m_GetWindowSizeFunc;
};

}
