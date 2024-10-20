#pragma once

#include "serious/vulkan/VulkanSyncs.hpp"
#include "serious/vulkan/VulkanBuffer.hpp"
#include "serious/vulkan/VulkanCommand.hpp"
#include "serious/vulkan/VulkanPipeline.hpp"
#include "serious/VulkanUtils.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>

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
class VulkanTextureImage;

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class VulkanRenderer final
{
public:
    VulkanRenderer(VulkanDevice* device, VulkanSwapchain* swapchain);
    ~VulkanRenderer();
    void Destroy();

    void OnUpdate();
private:
    void UpdateUniforms();
    void CreateFrameDatas();
    void DestroyFrameDatas();
private:
    VulkanDevice* m_Device;
    VulkanSwapchain* m_Swapchain;

    uint32_t m_CurrentFrame;
    uint32_t m_MaxFrame;    
    std::vector<VulkanFence> m_Fences;
    std::vector<VulkanSemaphore> m_ImageAvailableSems;
    std::vector<VulkanSemaphore> m_RenderFinishedSems;

    Ref<VulkanCommandPool> m_GfxCmdPool;
    std::vector<VulkanCommandBuffer> m_GfxCmdBufs;
    Ref<VulkanCommandPool> m_TsfCmdPool;

    std::array<VkClearValue, 2> m_ClearValues;
    Ref<VulkanRenderPass> m_RenderPass;
    std::vector<VulkanShaderModule> m_ShaderModules;
    Ref<VulkanPipeline> m_Pipeline;

    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_ImageViews;
    std::vector<VkFramebuffer> m_Framebuffers;
    
    Ref<VulkanDeviceBuffer> m_VertexBuffer;
    Ref<VulkanDeviceBuffer> m_IndexBuffer;
    std::vector<VulkanBuffer> m_UniformBuffers;
    std::vector<void*> m_UniformBufferMapped;
    std::vector<VkDescriptorSet> m_DescriptorSets;

    Ref<VulkanTextureImage> m_TextureImage;
    Ref<VulkanDepthImage> m_DepthImage;
};

}