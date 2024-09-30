#pragma once

#include <vulkan/vulkan.h>

#include <string>
#include <vector>

namespace serious
{

class VulkanDevice;
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

class VulkanPipelineLayout final
{
public:
    VulkanPipelineLayout(VulkanDevice* device);
    ~VulkanPipelineLayout();
    void Destroy();

    inline VkPipelineLayout GetHandle() const { return m_PipelineLayout; }
private:
    VkPipelineLayout m_PipelineLayout;
    VulkanDevice* m_Device;
};


class VulkanPipeline final
{
public:
    VulkanPipeline(
        VulkanDevice* device,
        const std::vector<VulkanShaderModule>& shaderModules,
        VulkanRenderPass& renderPass,
        uint32_t width,
        uint32_t height);
    ~VulkanPipeline();
    void Destroy();
    
    inline VkPipeline GetHandle() const { return m_Pipeline; }
private:
    VkPipeline m_Pipeline;
    VulkanPipelineLayout m_Layout;

    VulkanDevice* m_Device;
};

}