#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace serious
{

class VulkanDevice;
class VulkanSwapchain;
class VulkanRenderPass;

class VulkanShaderModule final
{
public:
    VulkanShaderModule(VulkanDevice* device, const std::string& file, VkShaderStageFlagBits flag);
    ~VulkanShaderModule();
    void Destroy();

    inline VkShaderStageFlagBits GetStage() const { return m_Stage; }
    inline VkShaderModule GetHandle() const { return m_ShaderModule; }
private:
    VkShaderModule m_ShaderModule;
    VulkanDevice* m_Device;
    VkShaderStageFlagBits m_Stage;
};

class VulkanPipeline final
{
public:
    VulkanPipeline(
        VulkanDevice* device,
        const std::vector<VulkanShaderModule>& shaderModules,
        VulkanRenderPass& renderPass,
        VulkanSwapchain& swapchain);
    ~VulkanPipeline();
    void Destroy();
    void AllocateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets);
    
    inline VkPipeline GetHandle() const { return m_Pipeline; }
    inline VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    inline VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
private:
    void CreateDescriptorSetLayout();
    void CreateDescriptorPool();
private:
    VkPipeline m_Pipeline;
    VulkanDevice* m_Device;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkPipelineLayout m_PipelineLayout;

    VkDescriptorPool m_DescriptorPool;
};

}
