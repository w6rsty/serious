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
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
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
    , m_GfxCmdPool(nullptr)
    , m_GfxCmdBufs({})
    , m_TsfCmdPool(nullptr)
    , m_ClearValue({ {{0.1f, 0.1f, 0.1f, 1.0f}} })
    , m_RenderPass(nullptr)
    , m_ShaderModules({})
    , m_Pipeline(nullptr)
    , m_Images({})
    , m_ImageViews({})
    , m_Framebuffers({})
    , m_VertexBuffer(nullptr)
    , m_IndexBuffer(nullptr)
    , m_UniformBuffers({})
    , m_UniformBufferMapped({})
    , m_DescriptorSets({})
    , m_TextureImage(nullptr)
{
    ZoneScopedN("VulkanRenderer Init");

    m_RenderPass = CreateRef<VulkanRenderPass>(device, *m_Swapchain);
    CreateFrameDatas();

    m_MaxFrame = m_Framebuffers.size();

    m_GfxCmdPool = CreateRef<VulkanCommandPool>(m_Device, m_Device->GetGraphicsQueue().get());
    m_TsfCmdPool = CreateRef<VulkanCommandPool>(m_Device, m_Device->GetTransferQueue().get());

    for (size_t i = 0; i < m_MaxFrame; ++i) {
        m_GfxCmdBufs.push_back(m_GfxCmdPool->Allocate());

        m_Fences.emplace_back(device, VK_FENCE_CREATE_SIGNALED_BIT);
        m_ImageAvailableSems.emplace_back(device);
        m_RenderFinishedSems.emplace_back(device);
    }

    m_ShaderModules.emplace_back(device, "shaders/vert.spv", VK_SHADER_STAGE_VERTEX_BIT);    
    m_ShaderModules.emplace_back(device, "shaders/frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);    

    m_Pipeline = CreateRef<VulkanPipeline>(device, m_ShaderModules, *m_RenderPass, *m_Swapchain);

    m_VertexBuffer = CreateRef<VulkanDeviceBuffer>(
        m_Device,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        sizeof(Vertex) * vertices.size(),
        vertices.data(),
        *m_TsfCmdPool
    );

    m_IndexBuffer = CreateRef<VulkanDeviceBuffer>(
        m_Device,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        sizeof(uint16_t) * indices.size(),
        indices.data(),
        *m_TsfCmdPool
    );

    // Uniform buffer(Init)
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
    m_TextureImage = CreateRef<VulkanTextureImage>(
        m_Device,
        *m_Swapchain,
        VK_IMAGE_LAYOUT_UNDEFINED,
        "textures/texture.jpg",
        *m_GfxCmdPool
    );
    {
        m_DescriptorSets.resize(m_MaxFrame, VK_NULL_HANDLE);
        m_Pipeline->AllocateDescriptorSets(m_DescriptorSets);

        for (uint32_t i = 0; i < m_MaxFrame; ++i) {
            VkDescriptorBufferInfo bufferInfo {};
            bufferInfo.buffer = m_UniformBuffers[i].GetHandle();
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_TextureImage->GetView();
            imageInfo.sampler = m_TextureImage->GetSampler();

            std::array<VkWriteDescriptorSet, 2> descriptorWrites {};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_DescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_DescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(
                m_Device->GetHandle(),
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr
            );
        }
    }
}

VulkanRenderer::~VulkanRenderer()
{
}

void VulkanRenderer::Destroy()
{
    m_Pipeline->Destroy();

    m_TextureImage->Destroy();
    
    for (VulkanBuffer& uniformBuffer : m_UniformBuffers) {
        uniformBuffer.Unmap();
        uniformBuffer.Destroy();
    }

    m_VertexBuffer->Destroy();
    m_IndexBuffer->Destroy();
    
    for (VulkanShaderModule& shaderModule : m_ShaderModules) {
        shaderModule.Destroy();
    }

    DestroyFrameDatas();
    m_RenderPass->Destroy();

    for (size_t i = 0; i < m_MaxFrame; ++i) {
        m_Fences[i].Destroy();
        m_ImageAvailableSems[i].Destroy();
        m_RenderFinishedSems[i].Destroy();
        m_GfxCmdPool->Free(m_GfxCmdBufs[i]);
    }
    
    m_GfxCmdPool->Destroy();
    m_TsfCmdPool->Destroy();
}

void VulkanRenderer::OnUpdate()
{
    UpdateUniforms();

    m_Fences[m_CurrentFrame].WaitAndReset();
    VulkanCommandBuffer& cmdBuf = m_GfxCmdBufs[m_CurrentFrame];
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

    cmdBuf.BeginSingle();
    m_RenderPass->CmdBegin(cmd, beginInfo);
    cmdBuf.BindGraphicsPipeline(*m_Pipeline);
    cmdBuf.BindVertexBuffer(m_VertexBuffer->GetHandle(), 0);
    cmdBuf.BindIndexBuffer(m_IndexBuffer->GetHandle(), 0, VK_INDEX_TYPE_UINT16);
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
    ubo.view = glm::lookAt(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
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
    imageViewInfo.components = VkComponentMapping {};
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
