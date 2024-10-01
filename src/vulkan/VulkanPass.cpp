#include "serious/vulkan/VulkanPass.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/vulkan/VulkanDevice.hpp"

namespace serious
{

VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, const VulkanSwapchain& swapchain)
    : m_RenderPass(VK_NULL_HANDLE)
    , m_Device(device)
{
    VkAttachmentDescription attachmentDesc = {};
    attachmentDesc.format = swapchain.GetColorFormat();
    attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
    attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference attachmentRef = {};
    attachmentRef.attachment = 0;
    attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &attachmentRef;

    VkSubpassDependency subpassDepend = {};
    subpassDepend.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDepend.dstSubpass = 0;
    subpassDepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDepend.srcAccessMask = 0;
    subpassDepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDepend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo passInfo = {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpassDesc;
    passInfo.attachmentCount = 1;
    passInfo.pAttachments = &attachmentDesc;
    passInfo.dependencyCount = 1;
    passInfo.pDependencies = &subpassDepend;

    VK_CHECK_RESULT(vkCreateRenderPass(device->GetHandle(), &passInfo, nullptr, &m_RenderPass));
}

VulkanRenderPass::~VulkanRenderPass()
{
}

void VulkanRenderPass::Destroy()
{
    vkDestroyRenderPass(m_Device->GetHandle(), m_RenderPass, nullptr);
}

void VulkanRenderPass::CmdBegin(
    VkCommandBuffer cmdBuf,
    VkFramebuffer framebuffer,
    VkClearValue* clearValue,
    VkRect2D area, 
    VkSubpassContents contents)
{
    VkRenderPassBeginInfo passBegin = {};
    passBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    passBegin.renderPass = m_RenderPass;
    passBegin.clearValueCount = 1;
    passBegin.pClearValues = clearValue;
    passBegin.framebuffer = framebuffer;
    passBegin.renderArea = area;

    vkCmdBeginRenderPass(cmdBuf, &passBegin, contents);
}

void VulkanRenderPass::CmdNext(VkCommandBuffer cmdBuf, VkSubpassContents contents)
{
    vkCmdNextSubpass(cmdBuf, contents);
}

void VulkanRenderPass::CmdEnd(VkCommandBuffer cmdBuf)
{
    vkCmdEndRenderPass(cmdBuf);
}

}