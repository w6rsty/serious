#pragma once

#include "serious/RHI.hpp"
#include "serious/vulkan/VulkanSyncs.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace serious
{

class VulkanDevice;
class VulkanRenderPass;
class VulkanCommandPool;
class VulkanCommandBuffer;
class VulkanWindow;

struct VulkanSwapchainRecreateInfo
{
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
};

class VulkanSwapchain final
{
public:
    VulkanSwapchain(VulkanDevice* device, VulkanWindow* window);
    ~VulkanSwapchain();

    void Create(VulkanSwapchainRecreateInfo* recreateInfo);
    void Destroy();
    void Present(VulkanSemaphore* outSemaphore);
    uint32_t AcquireNextImage(VulkanSemaphore* outSemaphore);
    void OnResize();
    void CreateFramebuffers(VulkanRenderPass& pass);
    void DestroyFrameBuffers();

    inline VkExtent2D GetExtent() const { return {m_WindowSpec.width, m_WindowSpec.height}; }
    inline VkFormat GetColorFormat() const { return m_ColorFormat; }
    inline VkImage GetImage(size_t index) const { return m_Images[index]; }
    inline VkImageView GetImageView(size_t index) const { return m_ImageViews[index]; }
    inline VkFramebuffer GetFramebuffer(size_t index) const { return m_Framebuffers[index]; }
    inline VkSwapchainKHR GetHandle() const { return m_Swapchain; }
private:
    VkSwapchainKHR m_Swapchain;
    VulkanDevice* m_Device;
    VulkanWindow* m_Window;

    uint32_t m_CurrentImageIndex;

    WindowSpec m_WindowSpec;
    VkFormat m_ColorFormat;
    VkColorSpaceKHR m_ColorSpace;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;
};

}
