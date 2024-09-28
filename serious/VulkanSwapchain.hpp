#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace serious
{

struct VulkanSwapchainRecreateInfo
{
    VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE;
};

class VulkanSwapchain final
{
public:
    VulkanSwapchain(uint32_t width, uint32_t height, bool vsync);
    ~VulkanSwapchain();

    void Create(uint32_t width, uint32_t height, bool vsync, VulkanSwapchainRecreateInfo* recreateInfo);
    void Destroy();

    inline VkSwapchainKHR GetHandle() const { return m_Swapchain; }
    inline uint32_t GetPresentId() const { return m_PresentId; }
private:
    VkSwapchainKHR m_Swapchain;

    VkFormat m_ColorFormat;
    VkColorSpaceKHR m_ColorSpace;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;

    uint32_t m_PresentId = 0;
};

}
