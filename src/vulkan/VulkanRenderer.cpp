#include "serious/vulkan/VulkanRenderer.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/vulkan/VulkanPass.hpp"
#include "serious/vulkan/VulkanCommand.hpp"
#include "serious/vulkan/Vertex.hpp"

#include <Tracy.hpp>

#include <chrono>

namespace serious
{

static const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}},
};

static const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

VulkanRenderer::VulkanRenderer(VulkanDevice* device, VulkanSwapchain* swapchain)
    : m_Device(device)
    , m_Swapchain(swapchain)
    , m_CurrentFrame(0)
    , m_MaxFrame(0)
    , m_Fences({})
    , m_ImageAvailableSems({})
    , m_RenderFinishedSems({})
    , m_CmdPool(nullptr)
    , m_CmdBufs({})
    , m_TransferCmdPool(nullptr)
    , m_ClearValue({ {{0.1f, 0.1f, 0.1f, 1.0f}} })
    , m_RenderPass(nullptr)
    , m_ShaderModules({})
    , m_Pipeline(nullptr)
    , m_Images({})
    , m_ImageViews({})
    , m_Framebuffers({})
    , m_VertexBuffer(m_Device)
    , m_IndexBuffer(m_Device)
    , m_UniformBuffers({})
    , m_UniformBufferMapped({})
    , m_DescriptorSets({})
{
    ZoneScopedN("VulkanRenderer Init");

    m_RenderPass = CreateRef<VulkanRenderPass>(device, *m_Swapchain);
    CreateFrameDatas();

    m_MaxFrame = m_Framebuffers.size();

    m_CmdPool = CreateRef<VulkanCommandPool>(m_Device, m_Device->GetGraphicsQueue().get());
    m_TransferCmdPool = CreateRef<VulkanCommandPool>(m_Device, m_Device->GetTransferQueue().get());

    for (size_t i = 0; i < m_MaxFrame; ++i) {
        m_CmdBufs.push_back(m_CmdPool->Allocate());

        m_Fences.emplace_back(device, VK_FENCE_CREATE_SIGNALED_BIT);
        m_ImageAvailableSems.emplace_back(device);
        m_RenderFinishedSems.emplace_back(device);
    }

    m_ShaderModules.emplace_back(device, "shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);    
    m_ShaderModules.emplace_back(device, "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);    

    m_Pipeline = CreateRef<VulkanPipeline>(device, m_ShaderModules, *m_RenderPass, *m_Swapchain);

    // Vertex buffer
    {
        VulkanBuffer stagingBuffer(m_Device);

        stagingBuffer.Create(
            sizeof(Vertex) * vertices.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,   
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        stagingBuffer.MapOnce(vertices.data(), sizeof(Vertex) * vertices.size());

        m_VertexBuffer.Create(
            sizeof(Vertex) * vertices.size(),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VulkanCommandBuffer transferBuf = m_TransferCmdPool->Allocate();
        m_VertexBuffer.Copy(stagingBuffer, sizeof(Vertex) * vertices.size(), transferBuf, *m_Device->GetTransferQueue());
        m_TransferCmdPool->Free(transferBuf);
        
        stagingBuffer.Destroy();
    }
    // Index Buffer
    {
        VulkanBuffer stagingBuffer(m_Device);

        stagingBuffer.Create(
            sizeof(uint16_t) * indices.size(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,   
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        stagingBuffer.MapOnce(indices.data(), sizeof(uint16_t) * indices.size());

        m_IndexBuffer.Create(
            sizeof(uint16_t) * indices.size(),
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        VulkanCommandBuffer transferBuf = m_TransferCmdPool->Allocate();
        m_IndexBuffer.Copy(stagingBuffer, sizeof(uint16_t) * indices.size(), transferBuf, *m_Device->GetTransferQueue());
        m_TransferCmdPool->Free(transferBuf);

        stagingBuffer.Destroy(); 
    }
    // Uniform buffer(Init)
    {
        VkDeviceSize uboSize = sizeof(UniformBufferObject);
        m_UniformBuffers.resize(m_MaxFrame, VulkanBuffer(m_Device));
        m_UniformBufferMapped.resize(m_MaxFrame, nullptr);
        
        for (uint32_t i = 0; i < m_MaxFrame; ++i) {
            VulkanBuffer& uniformBuffer = m_UniformBuffers[i];
            uniformBuffer.Create(
                uboSize,
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
            m_UniformBufferMapped[i] = uniformBuffer.MapTo(uboSize);
        }
    }
    {
        m_DescriptorSets.resize(m_MaxFrame, VK_NULL_HANDLE);
        m_Pipeline->AllocateDescriptorSets(m_DescriptorSets);

        for (uint32_t i = 0; i < m_MaxFrame; ++i) {
            VkDescriptorBufferInfo bufferInfo {};
            bufferInfo.buffer = m_UniformBuffers[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkWriteDescriptorSet descriptorWrite {};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = m_DescriptorSets[i];
            descriptorWrite.dstBinding = 0;
            descriptorWrite.dstArrayElement = 0;
            descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrite.descriptorCount = 1;
            descriptorWrite.pBufferInfo = &bufferInfo;

            vkUpdateDescriptorSets(m_Device->GetHandle(), 1, &descriptorWrite, 0, nullptr);
        }
    }
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::Destroy()
{
    m_Pipeline->Destroy();
    
    for (VulkanBuffer& uniformBuffer : m_UniformBuffers) {
        uniformBuffer.Unmap();
        uniformBuffer.Destroy();
    }

    m_VertexBuffer.Destroy();
    m_IndexBuffer.Destroy();
    
    for (VulkanShaderModule& shaderModule : m_ShaderModules) {
        shaderModule.Destroy();
    }

    DestroyFrameDatas();
    m_RenderPass->Destroy();

    for (size_t i = 0; i < m_MaxFrame; ++i) {
        m_Fences[i].Destroy();
        m_ImageAvailableSems[i].Destroy();
        m_RenderFinishedSems[i].Destroy();
        m_CmdPool->Free(m_CmdBufs[i]);
    }
    
    m_CmdPool->Destroy();
    m_TransferCmdPool->Destroy();
}

void VulkanRenderer::OnUpdate()
{
    UpdateUniforms();

    m_Fences[m_CurrentFrame].WaitAndReset();
    VulkanCommandBuffer& cmdBuf = m_CmdBufs[m_CurrentFrame];
    cmdBuf.Reset();

    uint32_t index;
    m_Swapchain->AcquireImage(&m_ImageAvailableSems[m_CurrentFrame], index);

    VkCommandBuffer cmd = cmdBuf.GetHandle();

    VkRenderPassBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = m_RenderPass->GetHandle();
    beginInfo.framebuffer = m_Framebuffers[index];
    beginInfo.renderArea.offset = {0, 0};
    beginInfo.renderArea.extent = m_Swapchain->GetExtent();
    beginInfo.clearValueCount = 1;
    beginInfo.pClearValues = &m_ClearValue;

    cmdBuf.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    m_RenderPass->CmdBegin(cmd, beginInfo);
    cmdBuf.BindGraphicsPipeline(*m_Pipeline);
    cmdBuf.BindVertexBuffer(m_VertexBuffer, 0);
    cmdBuf.BindIndexBuffer(m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16);
    cmdBuf.BindDescriptorSet(m_Pipeline->GetPipelineLayout(), m_DescriptorSets[m_CurrentFrame]);
    cmdBuf.DrawIndexed(indices.size(), indices.size() / 3, 0, 0, 0);
    m_RenderPass->CmdEnd(cmd);
    cmdBuf.End();

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    // Graphics queue submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = m_ImageAvailableSems[m_CurrentFrame].GetHandlePtr();
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = m_RenderFinishedSems[m_CurrentFrame].GetHandlePtr();

    m_Device->GetGraphicsQueue()->Submit(submitInfo, m_Fences[m_CurrentFrame].GetHandle());
    
    // Present queue submit
    m_Swapchain->Present(&m_RenderFinishedSems[m_CurrentFrame]);
    
    m_CurrentFrame = (m_CurrentFrame + 1) % m_MaxFrame;

    FrameMark;
}

void VulkanRenderer::UpdateUniforms()
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    
    memcpy(m_UniformBufferMapped[m_CurrentFrame], &ubo, sizeof(UniformBufferObject));
}

void VulkanRenderer::CreateFrameDatas()
{
    /// Get images
    uint32_t numSwapchainImages = 0;
    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_Device->GetHandle(), m_Swapchain->GetHandle(), &numSwapchainImages, nullptr));
    m_Images.resize(numSwapchainImages, VK_NULL_HANDLE);
    m_ImageViews.resize(m_Images.size(), VK_NULL_HANDLE);
    m_Framebuffers.resize(m_Images.size(), VK_NULL_HANDLE);

    VK_CHECK_RESULT(vkGetSwapchainImagesKHR(m_Device->GetHandle(), m_Swapchain->GetHandle(), &numSwapchainImages, m_Images.data()));

    /// Create image views
    VkImageViewCreateInfo imageViewInfo = {};
    imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = m_Swapchain->GetColorFormat();
    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = 1;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    for (size_t i = 0; i < m_Images.size(); i++) {
        imageViewInfo.image = m_Images[i];
        VK_CHECK_RESULT(vkCreateImageView(m_Device->GetHandle(), &imageViewInfo, nullptr, &m_ImageViews[i]));
    }

    VkDevice device = m_Device->GetHandle();

    VkExtent2D windowSpec = m_Swapchain->GetExtent();
    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = m_RenderPass->GetHandle();
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = windowSpec.width;
    framebufferInfo.height = windowSpec.height;
    framebufferInfo.layers = 1;
    for (size_t i = 0; i < m_Framebuffers.size(); ++i) {
        framebufferInfo.pAttachments = &m_ImageViews[i];
        VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferInfo, nullptr, &m_Framebuffers[i]));
    }
}

void VulkanRenderer::DestroyFrameDatas()
{
    VkDevice device = m_Device->GetHandle();
    for (VkFramebuffer& framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    m_Framebuffers.clear();

    for (VkImageView imageView : m_ImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    m_ImageViews.clear();
}

}
