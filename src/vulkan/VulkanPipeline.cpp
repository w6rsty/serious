#include "serious/vulkan/VulkanPipeline.hpp"
#include "serious/vulkan/Vertex.hpp"
#include "serious/VulkanUtils.hpp"

#include <array>

namespace serious
{

VulkanPipeline::VulkanPipeline(
        VulkanDevice* device,
        const std::vector<VulkanShaderModule>& shaderModules,
        VkRenderPass renderPass,
        VulkanSwapchain& swapchain)
    : m_Pipeline(VK_NULL_HANDLE)
    , m_Device(device)
    , m_PipelineLayout(VK_NULL_HANDLE)
{
    // Pipeline Creation
    auto vtxBindingDescriptions = Vertex::GetBindingDescription();
    auto vtxAttributeDescriptions = Vertex::GetAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vtxInputState {};
    vtxInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vtxInputState.vertexBindingDescriptionCount = 1;
    vtxInputState.pVertexBindingDescriptions = &vtxBindingDescriptions;
    vtxInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vtxAttributeDescriptions.size());
    vtxInputState.pVertexAttributeDescriptions = vtxAttributeDescriptions.data(); 

    VkPipelineInputAssemblyStateCreateInfo inputAsmState {};
    inputAsmState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmState.primitiveRestartEnable = VK_FALSE;
    inputAsmState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkExtent2D extent = swapchain.GetExtent();
    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = extent;
    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rastState {};
    rastState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastState.depthClampEnable = VK_FALSE;
    rastState.depthBiasEnable = VK_FALSE;
    rastState.rasterizerDiscardEnable = VK_FALSE;
    rastState.lineWidth = 1.0f;
    rastState.polygonMode = VK_POLYGON_MODE_FILL;
    rastState.cullMode = VK_CULL_MODE_BACK_BIT;
    rastState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multiSampleState {};
    multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo colorBlendState = {};
    colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;
    colorBlendState.logicOpEnable = VK_FALSE;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos(shaderModules.size());
    for (size_t i = 0; i <  shaderStageInfos.size(); ++i) {
        VkPipelineShaderStageCreateInfo& shaderStageInfo = shaderStageInfos[i];
        shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStageInfo.module = shaderModules[i].handle;
        shaderStageInfo.stage = shaderModules[i].stage;
        shaderStageInfo.pName = "main";
    }

    VkPipelineDepthStencilStateCreateInfo depthStencilState {};
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.depthTestEnable = VK_TRUE;
    depthStencilState.depthWriteEnable = VK_TRUE;
    depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilState.depthBoundsTestEnable = VK_FALSE;

    VkDescriptorSetLayout descriptorsetLayout = m_Device->GetDescriptorSetLayout();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorsetLayout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_Device->GetHandle(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout));

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pVertexInputState = &vtxInputState;
    pipelineInfo.pInputAssemblyState = &inputAsmState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rastState;
    pipelineInfo.pMultisampleState = &multiSampleState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.stageCount = shaderStageInfos.size();
    pipelineInfo.pStages = shaderStageInfos.data();
    pipelineInfo.pDepthStencilState = &depthStencilState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = renderPass;
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_Device->GetHandle(), nullptr, 1, &pipelineInfo, nullptr, &m_Pipeline));
}

VulkanPipeline::~VulkanPipeline()
{
}

void VulkanPipeline::Destroy()
{
    m_Device->GetGraphicsQueue()->WaitIdle();
    m_Device->GetPresentQueue()->WaitIdle();
    m_Device->GetTransferQueue()->WaitIdle();

    VkDevice deviceHandle = m_Device->GetHandle();

    vkDestroyPipelineLayout(deviceHandle, m_PipelineLayout, nullptr);
    vkDestroyPipeline(deviceHandle, m_Pipeline, nullptr);
}

// void VulkanPipeline::AllocateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets)
// {
//     std::vector<VkDescriptorSetLayout> layouts(3, m_DescriptorSetLayout);

//     VkDescriptorSetAllocateInfo allocInfo {};
//     allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//     allocInfo.descriptorPool = m_DescriptorPool;
//     allocInfo.descriptorSetCount = 3;
//     allocInfo.pSetLayouts = layouts.data();
//     VK_CHECK_RESULT(vkAllocateDescriptorSets(m_Device->GetHandle(), &allocInfo, descriptorSets.data()));
// }

// void VulkanPipeline::CreateDescriptorPool()
// {
//     std::array<VkDescriptorPoolSize, 2> poolSizes {};
//     poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//     poolSizes[0].descriptorCount = 3;
//     poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//     poolSizes[1].descriptorCount = 3;
    
//     VkDescriptorPoolCreateInfo poolInfo {};
//     poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//     poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//     poolInfo.pPoolSizes = poolSizes.data();
//     poolInfo.maxSets = 3;
//     VK_CHECK_RESULT(vkCreateDescriptorPool(m_Device->GetHandle(), &poolInfo, nullptr, &m_DescriptorPool));
// }
    
}
