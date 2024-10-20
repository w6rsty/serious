#pragma once

#include "serious/RHI.hpp"
#include "serious/vulkan/VulkanSyncs.hpp"

#include <vulkan/vulkan.h>

#include <functional>

namespace serious
{

#define INVALID_IMAGE_INDEX UINT32_MAX

class VulkanDevice;
class VulkanRenderPass;
class VulkanCommandPool;
class VulkanCommandBuffer;
class VulkanWindow;

using SwapchainRecreationCallback = std::function<void()>;

class VulkanSwapchain final
{
public:
    VulkanSwapchain(VulkanDevice* device, VulkanWindow* window);
    ~VulkanSwapchain();

    void Create();
    void Destroy();
    void Present(VulkanSemaphore* outSemaphore);
    void AcquireImage(VulkanSemaphore* outSemaphore, uint32_t& imageIndex);

    inline VkExtent2D         GetExtent() const { return {m_WindowSpec.width, m_WindowSpec.height}; }
    inline VkFormat           GetColorFormat() const { return m_ColorFormat; }
    inline VkFormat           GetDepthFormat() const { return m_DepthFormat; }
    inline VkComponentMapping GetComponentMapping() const { return m_ComponentMapping; }
    inline VkSwapchainKHR     GetHandle() const { return m_Swapchain; }
private:
    VkSwapchainKHR m_Swapchain;
    VulkanDevice* m_Device;
    VulkanWindow* m_Window;

    WindowSpec m_WindowSpec;
    VkFormat m_ColorFormat;
    VkColorSpaceKHR m_ColorSpace;
    VkComponentMapping m_ComponentMapping;
    VkFormat m_DepthFormat;
    uint32_t m_CurrentImage;
};

}
