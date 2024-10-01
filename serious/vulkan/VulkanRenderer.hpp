#pragma once

#include "serious/vulkan/VulkanSyncs.hpp"
#include "serious/vulkan/VulkanPipeline.hpp"
#include "serious/VulkanUtils.hpp"

namespace serious
{

class VulkanDevice;
class VulkanPipeline;
class VulkanSwapchain;
class VulkanRenderPass;
class VulkanCommandPool;
class VulkanShaderModule;
class VulkanCommandBuffer;
class VulkanPipelineLayout;

class VulkanRenderer final
{
public:
    VulkanRenderer(VulkanDevice* device, VulkanSwapchain* swapchain);
    ~VulkanRenderer();
    void Destroy();

    void OnUpdate();
private:
    VulkanDevice* m_Device;
    VulkanSwapchain* m_Swapchain;
    
    VulkanFence m_Fence;
    VulkanSemaphore m_ImageAvailableSem;
    VulkanSemaphore m_RenderFinishedSem;

    Ref<VulkanCommandBuffer> m_CmdBuf;
    Ref<VulkanCommandPool> m_CmdPool;

    VkClearValue m_ClearValue;
    Ref<VulkanRenderPass> m_RenderPass;
    std::vector<VulkanShaderModule> m_ShaderModules;
    Ref<VulkanPipelineLayout> m_PipelineLayout; 
    Ref<VulkanPipeline> m_Pipeline;
};

}