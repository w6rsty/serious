#pragma once

#include "serious/RHI.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"

#include <vulkan/vulkan.h>

#include <vector>

namespace serious
{

class VulkanPipeline final : public SEPipeline
{
public:
    VulkanPipeline(VulkanDevice* device,
                   const std::vector<VulkanShaderModule>& shaders,
                   VkRenderPass renderPass,
                   VulkanSwapchain& swapchain);
    ~VulkanPipeline();
    void Destroy();
    
    inline VkPipeline GetHandle() const { return m_Pipeline; }
    inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
private:
    VkPipeline m_Pipeline;
    VulkanDevice* m_Device;
    VkPipelineLayout m_PipelineLayout;
};

}
