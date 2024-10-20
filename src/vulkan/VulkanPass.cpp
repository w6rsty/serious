#include "serious/vulkan/VulkanPass.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/vulkan/VulkanDevice.hpp"

#include <array>

namespace serious
{

VulkanRenderPass::VulkanRenderPass(VulkanDevice* device, const VulkanSwapchain& swapchain)
    : m_RenderPass(VK_NULL_HANDLE)
    , m_Device(device)
{
    // Color attachment
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = swapchain.GetColorFormat();
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment
    VkAttachmentDescription depthAttachment {};
    depthAttachment.format = swapchain.GetDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency subpassDepend {};
    subpassDepend.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDepend.dstSubpass = 0;
    subpassDepend.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDepend.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    subpassDepend.srcAccessMask = 0;
    subpassDepend.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    subpassDepend.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo passInfo {};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = &subpassDesc;
    passInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    passInfo.pAttachments = attachments.data();
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

void VulkanRenderPass::CmdBegin(VkCommandBuffer cmdBuf, VkRenderPassBeginInfo& beginInfo, VkSubpassContents contents)
{
    vkCmdBeginRenderPass(cmdBuf, &beginInfo, contents);
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