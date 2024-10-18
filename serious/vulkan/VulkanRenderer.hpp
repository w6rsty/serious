#pragma once

#include "serious/VulkanUtils.hpp"
#include "serious/vulkan/VulkanSyncs.hpp"
#include "serious/vulkan/VulkanBuffer.hpp"
#include "serious/vulkan/VulkanCommand.hpp"
#include "serious/vulkan/VulkanPipeline.hpp"

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

    void CreateFramebuffers();
    void DestroyFrameBuffers();
private:
    VulkanDevice* m_Device;
    VulkanSwapchain* m_Swapchain;

    uint32_t m_CurrentFrame;
    uint32_t m_MaxFrame;    
    std::vector<VulkanFence> m_Fences;
    std::vector<VulkanSemaphore> m_ImageAvailableSems;
    std::vector<VulkanSemaphore> m_RenderFinishedSems;

    Ref<VulkanCommandPool> m_CmdPool;
    std::vector<VulkanCommandBuffer> m_CmdBufs;
    Ref<VulkanCommandPool> m_TransferCmdPool;

    VkClearValue m_ClearValue;
    Ref<VulkanRenderPass> m_RenderPass;
    std::vector<VulkanShaderModule> m_ShaderModules;
    Ref<VulkanPipelineLayout> m_PipelineLayout; 
    Ref<VulkanPipeline> m_Pipeline;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;
    
    VulkanBuffer m_VertexBuffer;
    VulkanBuffer m_IndexBuffer;
};

}