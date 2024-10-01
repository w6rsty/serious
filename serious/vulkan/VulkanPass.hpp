#pragma once

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanDevice;
class VulkanSwapchain;

class VulkanRenderPass final
{
public:
    VulkanRenderPass(VulkanDevice* device, const VulkanSwapchain& swapchain);
    ~VulkanRenderPass();
    void Destroy();

    void CmdBegin(
        VkCommandBuffer cmdBuf,
        VkFramebuffer framebuffer,
        VkClearValue* clearValue,
        VkRect2D area,
        VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE);
    void CmdNext(VkCommandBuffer cmdBuf, VkSubpassContents contents);
    void CmdEnd(VkCommandBuffer cmdBuf);

    inline VkRenderPass GetHandle() const { return m_RenderPass; }

private:
    VkRenderPass m_RenderPass;
    VulkanDevice* m_Device;
};

}