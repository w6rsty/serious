#pragma once

#include "serious/VulkanUtils.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanPipelineLayout final
{
public:
    VulkanPipelineLayout();
    ~VulkanPipelineLayout();
    void Destroy();

    inline VkPipelineLayout GetHandle() const { return m_PipelineLayout; }
private:
    VkPipelineLayout m_PipelineLayout;
};

class VulkanPipeline final
{
public:
    VulkanPipeline();
    ~VulkanPipeline();
    void Destroy();
    
    inline VkPipeline GetHandle() const { return m_Pipeline; }
private:
    VkPipeline m_Pipeline;
    Ref<VulkanPipelineLayout> m_Layout;
};

}