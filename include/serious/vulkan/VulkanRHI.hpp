#pragma once

#include "serious/RHI.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/vulkan/VulkanPipeline.hpp"
#include "serious/vulkan/VulkanCommand.hpp"
#include "serious/vulkan/Vertex.hpp"
#include "serious/VulkanUtils.hpp"

namespace serious
{

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class VulkanRHI final : public RHI
{
public:
    VulkanRHI(const Settings& settings);
    virtual ~VulkanRHI() = default;
    virtual void Init(void* window) override;
    virtual void Shutdown() override;
    virtual void Update() override;
private:
    void CreateInstance();
    void CreateCommandPool();
    void CreateSyncObjects();
    void CreateRenderPass();
    void CreateFramebuffers();
    void SetDescriptorResources();
    void LoadObj(const std::string& path);
private:
    Settings m_Settings;

    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
    Ref<VulkanDevice> m_Device;
    VulkanSwapchain m_Swapchain;
    uint32_t m_SwapchainImageCount;

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
    VulkanBuffer m_VertexBuffer;
    VulkanBuffer m_IndexBuffer;
    std::vector<VulkanBuffer> m_UniformBuffers;
    std::vector<void*> m_UniformBufferMapped;
    Ref<VulkanPipeline> m_Pipeline;
    
    std::vector<Vertex> m_Vertices;
    std::vector<uint32_t> m_Indices;
};

}