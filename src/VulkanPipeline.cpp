#include "serious/VulkanPipeline.hpp"
#include "serious/VulkanDevice.hpp"
#include "serious/VulkanPass.hpp"
#include "serious/VulkanUtils.hpp"

namespace serious
{

VulkanShaderModule::VulkanShaderModule(VulkanDevice* device, const std::string& file, VkShaderStageFlagBits stage)
    : m_ShaderModule(VK_NULL_HANDLE)
    , m_Device(device)
    , m_Stage(stage)
{
    std::string source = ReadFile(file);
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = source.size();
    shaderModuleInfo.pCode = (uint32_t*)source.data();
    VK_CHECK_RESULT(vkCreateShaderModule(m_Device->GetHandle(), &shaderModuleInfo, nullptr, &m_ShaderModule));
}

VulkanShaderModule::~VulkanShaderModule()
{
}

void VulkanShaderModule::Destroy()
{
    if (m_ShaderModule) {
        vkDestroyShaderModule(m_Device->GetHandle(), m_ShaderModule, nullptr);
    }
}


VulkanPipelineLayout::VulkanPipelineLayout(VulkanDevice* device)
    : m_PipelineLayout(VK_NULL_HANDLE)
    , m_Device(device)
{
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_Device->GetHandle(), &layoutInfo, nullptr, &m_PipelineLayout));
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
}

void VulkanPipelineLayout::Destroy()
{
    if (m_PipelineLayout) {
        vkDestroyPipelineLayout(m_Device->GetHandle(), m_PipelineLayout, nullptr);
    }
}

VulkanPipeline::VulkanPipeline(
        VulkanDevice* device,
        const std::vector<VulkanShaderModule>& shaderModules,
        VulkanRenderPass& renderPass,
        uint32_t width,
        uint32_t height)
    : m_Pipeline(VK_NULL_HANDLE)
    , m_Layout(device)
    , m_Device(device)
{

    VkPipelineVertexInputStateCreateInfo vtxInputState = {};
    vtxInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAsmState = {};
    inputAsmState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAsmState.primitiveRestartEnable = VK_FALSE;
    inputAsmState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    VkRect2D scissor;
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rastState = {};
    rastState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rastState.depthClampEnable = VK_FALSE;
    rastState.depthBiasEnable = VK_FALSE;
    rastState.rasterizerDiscardEnable = VK_FALSE;
    rastState.lineWidth = 1.0f;
    rastState.polygonMode = VK_POLYGON_MODE_FILL;
    rastState.cullMode = VK_CULL_MODE_BACK_BIT;
    rastState.frontFace = VK_FRONT_FACE_CLOCKWISE;

    VkPipelineMultisampleStateCreateInfo multiSampleState = {};
    multiSampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
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
        shaderStageInfo.module = shaderModules[i].GetHandle();
        shaderStageInfo.stage = shaderModules[i].GetStage();
        shaderStageInfo.pName = "main";
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pVertexInputState = &vtxInputState;
    pipelineInfo.pInputAssemblyState = &inputAsmState;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rastState;
    pipelineInfo.pMultisampleState = &multiSampleState;
    pipelineInfo.pColorBlendState = &colorBlendState;
    pipelineInfo.stageCount = shaderStageInfos.size();
    pipelineInfo.pStages = shaderStageInfos.data();
    pipelineInfo.layout = m_Layout.GetHandle();
    pipelineInfo.renderPass = renderPass.GetHandle();
    VK_CHECK_RESULT(vkCreateGraphicsPipelines(m_Device->GetHandle(), nullptr, 1, &pipelineInfo, nullptr, &m_Pipeline));
}

VulkanPipeline::~VulkanPipeline()
{
}

void VulkanPipeline::Destroy()
{
    m_Layout.Destroy();
    if (m_Pipeline) {
        vkDestroyPipeline(m_Device->GetHandle(), m_Pipeline, nullptr);
    }
}
    
}