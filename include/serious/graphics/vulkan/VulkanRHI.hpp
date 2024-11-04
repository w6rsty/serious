#pragma once

#include "serious/graphics/Objects.hpp"
#include "serious/graphics/RHI.hpp"
#include "serious/graphics/vulkan/VulkanDevice.hpp"
#include "serious/graphics/vulkan/VulkanSwapchain.hpp"
#include "serious/graphics/vulkan/VulkanCommand.hpp"
#include "serious/graphics/vulkan/VulkanPipeline.hpp"

#include "serious/graphics/Camera.hpp"

namespace serious
{

class VulkanRHI final : public RHI
{
public:
    VulkanRHI(const Settings& settings);
    virtual ~VulkanRHI() = default;
    virtual void Init(void* window) override;
    virtual bool AssureResource() override;
    virtual void Shutdown() override;
    virtual void PrepareFrame() override;
    virtual void SubmitFrame() override;
    virtual void Update() override;
    // Deferred buffer creation
    virtual RHIResourceIdx CreateShader(const ShaderDescription& description) override;
    virtual RHIResource CreatePipeline(const PipelineDescription& description) override;
    virtual RHIResourceIdx CreateBuffer(const BufferDescription& description) override;
    virtual void BindPipeline(RHIResource pipeline) override;
    virtual void DestroyPipeline(RHIResource pipeline) override;
    virtual Camera& GetCamera() override { return m_Camera; }

    virtual void SetPasses(const std::vector<RenderPassDescription>& descriptions) override;

    virtual void SetClearColor(float r, float g, float b, float a) override;
    virtual void SetClearDepth(float depth) override;
private:
    virtual void WindowResize() override;
    void CreateInstance();
    void CreateCommandPool();
    void CreateSyncObjects();
    void CreateRenderPass();
    void CreateFramebuffers();
    void SetDescriptorResources();
    void UpdateUniforms();
private:
    Settings m_Settings;

    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
    Ref<VulkanDevice> m_Device;
    void* m_PlatformWindow;
    VulkanSwapchain m_Swapchain;
    uint32_t m_SwapchainImageCount;
    uint32_t m_SwapchainImageIndex;

    VulkanCommandPool m_GfxCmdPool;
    VulkanCommandPool m_TsfCmdPool;
    std::vector<VulkanCommandBuffer> m_GfxCmdBufs;

    uint32_t m_CurrentFrame;
    std::vector<VulkanFence> m_Fences;
    std::vector<VkSemaphore> m_ImageAvailableSems;
    std::vector<VkSemaphore> m_RenderFinishedSems;

    VulkanTexture m_DepthImage;
    VkRenderPass m_RenderPass;
    std::vector<VkFramebuffer> m_Framebuffers;
    std::vector<VulkanShaderModule> m_ShaderModules;
    std::vector<VkDescriptorSet> m_DescriptorSets;
    VulkanTexture m_TextureImage;
    VkClearValue m_ClearValues[2];
    std::vector<VulkanBuffer> m_UniformBuffers;
    std::vector<void*> m_UniformBufferMapped;
    VulkanPipeline* m_BoundPipline;
    VkViewport m_Viewport;
    VkRect2D m_Scissor;

    std::vector<BufferDescription> m_BufferDescriptions;
    std::vector<VulkanBuffer> m_Buffers;
    std::vector<RenderPassDescription> m_PassDescriptions;

    Camera m_Camera;
};

}