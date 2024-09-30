#include "serious/Renderer.hpp"
#include "serious/VulkanDevice.hpp"
#include "serious/VulkanSwapchain.hpp"
#include "serious/VulkanPass.hpp"

namespace serious
{

Renderer::Renderer(VulkanDevice* device, VulkanSwapchain* swapchain)
    : m_Device(device)
    , m_Swapchain(swapchain)
    , m_Fence(device, VK_FENCE_CREATE_SIGNALED_BIT)
    , m_ImageAvailableSem(device)
    , m_RenderFinishedSem(device)
    , m_CmdBuf(nullptr)
    , m_CmdPool(nullptr)
    , m_ClearValue({ {{0.1f, 0.1f, 0.1f, 1.0f}} })
    , m_RenderPass(nullptr)
    , m_ShaderModules({})
    , m_PipelineLayout(nullptr)
    , m_Pipeline(nullptr)
{
    m_CmdPool = CreateRef<VulkanCommandPool>(m_Device);
    m_CmdPool->Create(m_Device->GetGraphicsQueue().get());

    m_CmdBuf = CreateRef<VulkanCommandBuffer>(device, m_CmdPool);
    m_CmdBuf->Allocate();

    m_RenderPass = CreateRef<VulkanRenderPass>(device);
    m_Swapchain->CreateFramebuffers(m_RenderPass.get());

    m_ShaderModules.emplace_back(device, "shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);    
    m_ShaderModules.emplace_back(device, "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);    

    m_PipelineLayout = CreateRef<VulkanPipelineLayout>(device);
    VkExtent2D size = m_Swapchain->GetExtent();
    m_Pipeline = CreateRef<VulkanPipeline>(device, m_ShaderModules, *m_RenderPass, size.width, size.height);
}

Renderer::~Renderer()
{
    VK_CHECK_RESULT(vkQueueWaitIdle(m_Device->GetGraphicsQueue()->GetHandle()));
    VK_CHECK_RESULT(vkQueueWaitIdle(m_Device->GetPresentQueue()->GetHandle()));
    
    m_Pipeline->Destroy();
    m_PipelineLayout->Destroy();
    for (VulkanShaderModule& shaderModule : m_ShaderModules) {
        shaderModule.Destroy();
    }

    m_Swapchain->DestroyFrameBuffers();
    m_RenderPass->Destroy();

    m_CmdBuf->Free();
    m_CmdPool->Destroy();

    m_Fence.Destroy();
    m_ImageAvailableSem.Destroy();
    m_RenderFinishedSem.Destroy();
}

void Renderer::OnUpdate()
{
    m_Fence.WaitAndReset();

    uint32_t index = m_Swapchain->AcquireNextImage(&m_ImageAvailableSem);

    VkCommandBuffer cmd = m_CmdBuf->GetHandle();

    m_CmdBuf->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    m_RenderPass->CmdBegin(cmd, m_Swapchain->GetFramebuffer(index), &m_ClearValue, VkRect2D {{0, 0}, m_Swapchain->GetExtent()});
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetHandle());
    vkCmdDraw(cmd, 3, 1, 0, 0);
    m_RenderPass->CmdEnd(cmd);
    m_CmdBuf->End();

    SubmitGraphics(m_CmdBuf.get(), &m_ImageAvailableSem, &m_RenderFinishedSem, &m_Fence);
    
    m_Swapchain->Present(&m_RenderFinishedSem);
}

void Renderer::SubmitGraphics(VulkanCommandBuffer* cmdBuf, VulkanSemaphore* imageAvailableSem, VulkanSemaphore* renderFinishedSem, VulkanFence* fence)
{
    VkCommandBuffer cmdBufHandle = cmdBuf->GetHandle();
    VkSemaphore imageAvailableSemHandle = imageAvailableSem->GetHandle();
    VkSemaphore renderFinishedSemHandle = renderFinishedSem->GetHandle();
    VkFence fenceHandle = fence->GetHandle();

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailableSemHandle;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBufHandle;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinishedSemHandle;

    VK_CHECK_RESULT(vkQueueSubmit(m_Device->GetGraphicsQueue()->GetHandle(), 1, &submitInfo, fenceHandle));
}

}