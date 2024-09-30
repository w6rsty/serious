#pragma once

#include "serious/VulkanSyncs.hpp"
#include "serious/VulkanUtils.hpp"
#include "serious/VulkanPipeline.hpp"

namespace serious
{

class VulkanDevice;
class VulkanSwapchain;
class VulkanCommandBuffer;
class VulkanCommandPool;
class VulkanRenderPass;
class VulkanShaderModule;
class VulkanPipelineLayout;
class VulkanPipeline;

class Renderer final
{
public:
    Renderer(VulkanDevice* device, VulkanSwapchain* swapchain);
    ~Renderer();

    void OnUpdate();
private:
    void SubmitGraphics(
        VulkanCommandBuffer* cmdBuf,
        VulkanSemaphore* imageAvailableSem,
        VulkanSemaphore* renderFinishedSem,
        VulkanFence* fence);
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