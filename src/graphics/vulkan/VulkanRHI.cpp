#include "serious/graphics/vulkan/VulkanRHI.hpp"
#include "serious/graphics/Objects.hpp"
#include "serious/graphics/vulkan/VulkanDevice.hpp"

#include <string>
#include <array>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Tracy.hpp>

namespace serious
{

// --------------
// DebugMessenger
// --------------
constexpr const char* VulkanDebugUtilsMessageSeverity(const VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:   return "VERBOSE";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:      return "INFO";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:   return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:     return "ERROR";
        default:                                                return "UNKNOWN";
    }
}
   
static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    (void)pUserData;
    (void)messageType;
    SEWarn("[{}] {}", VulkanDebugUtilsMessageSeverity(messageSeverity), pCallbackData->pMessage);
    return VK_FALSE;
}

static VkDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT& creaetInfo)
{
    static PFN_vkCreateDebugUtilsMessengerEXT fpCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    VK_CHECK_RESULT(fpCreateDebugUtilsMessengerEXT(instance, &creaetInfo, nullptr, &debugUtilsMessenger));
    return debugUtilsMessenger;
}

static inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger)
{
    static PFN_vkDestroyDebugUtilsMessengerEXT fpDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (debugUtilsMessenger != VK_NULL_HANDLE) {
        fpDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
    }
}

// ----------
// Vulkan RHI
// ----------
VulkanRHI::VulkanRHI(const Settings& settings)
    : m_Settings(settings)
    , m_Instance(VK_NULL_HANDLE)
    , m_DebugUtilsMessenger(VK_NULL_HANDLE)
    , m_Device(nullptr)
    , m_PlatformWindow(nullptr)
    , m_Swapchain({})
    , m_SwapchainImageCount(0)
    , m_SwapchainImageIndex(0)
    , m_GfxCmdPool({})
    , m_TsfCmdPool({})
    , m_GfxCmdBufs({})
    , m_CurrentFrame(0)
    , m_Fences({})
    , m_ImageAvailableSems({})
    , m_RenderFinishedSems({})
    , m_DepthImage({})
    , m_RenderPass(VK_NULL_HANDLE)
    , m_Framebuffers({})
    , m_ShaderModules({})
    , m_DescriptorSets({})
    , m_TextureImage({})
    , m_ClearValues{ {}, {} }
    , m_UniformBuffers({})
    , m_UniformBufferMapped({})
    , m_BoundPipline(nullptr)
    , m_Viewport({})
    , m_Scissor({})
{
}

void VulkanRHI::Init(void* window)
{
    CreateInstance();
    m_Device = CreateRef<VulkanDevice>(m_Instance);

    // Create swapchain
    m_Swapchain.SetContext(m_Instance, m_Device.get());
    m_Swapchain.InitSurface(window);
    m_PlatformWindow = window;
    m_Swapchain.Create(&m_Settings.height, &m_Settings.height, m_Settings.vsync);
    m_SwapchainImageCount = m_Swapchain.GetImageCount();
    VkExtent2D extent = m_Swapchain.GetExtent();
    m_Viewport.x = 0.0f;
    m_Viewport.y = 0.0f;
    m_Viewport.width = extent.width;
    m_Viewport.height = extent.height;
    m_Viewport.minDepth = 0.0f;
    m_Viewport.maxDepth = 1.0f;
    m_Scissor.offset = {0, 0};
    m_Scissor.extent = extent;

    CreateCommandPool();
    CreateSyncObjects();

    // Pipeline resources
    auto gfxCmd = m_GfxCmdPool.Allocate();
    m_Device->CreateDepthImage(m_DepthImage, m_Swapchain.GetExtent(), gfxCmd);
    m_GfxCmdPool.Free(gfxCmd);

    CreateRenderPass();
    CreateFramebuffers();
    SetDescriptorResources();

    m_Camera.SetPerspective(45.0f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 1000.0f);
    m_Camera.SetPosition(glm::vec3(0.0f, 0.0f, -2.0f));
}

bool VulkanRHI::AssureResource()
{   
    auto tsfCmd = m_TsfCmdPool.Allocate();
    for (size_t i = 0; i < m_Buffers.size(); ++i) {
        VulkanBuffer& buffer = m_Buffers[i];
        BufferDescription& description = m_BufferDescriptions[i];
        VkBufferUsageFlags usageFlag;
        switch (description.usage) {
            case BufferUsage::Vertex:
                usageFlag = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                break;
            case BufferUsage::Index:
                usageFlag = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                break;
            case BufferUsage::Uniform:
                usageFlag = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                break;
        }
        m_Device->CreateDeviceBuffer(
            buffer,
            description.size,
            description.data,
            usageFlag,
            tsfCmd
        );

    }
    m_TsfCmdPool.Free(tsfCmd);

    return true;
}

void VulkanRHI::Shutdown()
{    
    VkDevice device = m_Device->GetHandle();
    m_Device->WaitIdle();

    m_Device->DestroyTextureImage(m_TextureImage);

    m_Device->DestroyDescriptorResources();
    for (VulkanBuffer& buffer : m_UniformBuffers) {
        m_Device->DestroyBuffer(buffer);
    }

    for (VulkanBuffer& buffer : m_Buffers) {
        m_Device->DestroyBuffer(buffer);
    }

    for (VulkanShaderModule& shaderModule : m_ShaderModules) {
        m_Device->DestroyShaderModule(shaderModule);
    }
    for (VkFramebuffer& framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    vkDestroyRenderPass(device, m_RenderPass, nullptr);

    m_Device->DestroyTextureImage(m_DepthImage);

    for (uint32_t i = 0; i < m_Swapchain.GetImageCount(); ++i) {
        m_Device->DestroyFence(m_Fences[i]);
        vkDestroySemaphore(device, m_ImageAvailableSems[i], nullptr);
        vkDestroySemaphore(device, m_RenderFinishedSems[i], nullptr);
    }
    
    m_Device->DestroyCommandPool(m_GfxCmdPool);
    m_Device->DestroyCommandPool(m_TsfCmdPool);
    
    m_Swapchain.Cleanup();
    // Core resources
    m_Device->Destroy();
    if (m_Settings.validation) {
        DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugUtilsMessenger);
    }
    vkDestroyInstance(m_Instance, nullptr);
}

void VulkanRHI::WindowResize()
{
    m_Device->WaitIdle();
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(static_cast<SDL_Window*>(m_PlatformWindow), &width, &height);
    m_Settings.width = static_cast<uint32_t>(width);
    m_Settings.height = static_cast<uint32_t>(height);
    m_Swapchain.Create(&m_Settings.width, &m_Settings.height, m_Settings.vsync);
    m_Camera.SetPerspective(m_Camera.fov, static_cast<float>(m_Settings.width) / static_cast<float>(m_Settings.height), m_Camera.zNear, m_Camera.zFar);

    m_Device->DestroyTextureImage(m_DepthImage);
    auto gfxCmd = m_GfxCmdPool.Allocate();
    m_Device->CreateDepthImage(m_DepthImage, m_Swapchain.GetExtent(), gfxCmd);
    m_GfxCmdPool.Free(gfxCmd);

    for (VkFramebuffer& framebuffer : m_Framebuffers) {
        vkDestroyFramebuffer(m_Device->GetHandle(), framebuffer, nullptr);
    }
    CreateFramebuffers();
}

void VulkanRHI::PrepareFrame()
{
    ZoneScoped;

    VkResult result = m_Swapchain.AcquireNextImage(m_ImageAvailableSems[m_CurrentFrame], &m_SwapchainImageIndex);
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			WindowResize();
		}
		return;
	}	
	else {
		VK_CHECK_RESULT(result);
	}
}

void VulkanRHI::SubmitFrame()
{
    ZoneScoped;

    VkResult result = m_Swapchain.Present(&m_RenderFinishedSems[m_CurrentFrame], m_SwapchainImageIndex);
	if ((result == VK_ERROR_OUT_OF_DATE_KHR) || (result == VK_SUBOPTIMAL_KHR)) {
		WindowResize();
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}
	}
	else {
		VK_CHECK_RESULT(result);
	}
}

void VulkanRHI::Update()
{
    UpdateUniforms();

    m_Fences[m_CurrentFrame].WaitAndReset();

    PrepareFrame();

    auto gfxCmd = m_GfxCmdBufs[m_CurrentFrame];    
    gfxCmd.BeginSingle();
    {
        VkExtent2D extent = m_Swapchain.GetExtent();
        m_Viewport.width = static_cast<float>(extent.width);
        m_Viewport.height = static_cast<float>(extent.height);
        gfxCmd.SetViewport(m_Viewport);
        m_Scissor.extent = extent;
        gfxCmd.SetScissor(m_Scissor);
 
        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass = m_RenderPass;
        beginInfo.framebuffer = m_Framebuffers[m_SwapchainImageIndex];
        beginInfo.renderArea = m_Scissor;
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = m_ClearValues;

        for (const auto& pass : m_PassDescriptions) {
            gfxCmd.BeginRenderPass(beginInfo, VK_SUBPASS_CONTENTS_INLINE);
            gfxCmd.BindGraphicsPipeline(m_BoundPipline->GetHandle());
            gfxCmd.BindVertexBuffer(m_Buffers[pass.vertexBuffer].buffer, 0);
            gfxCmd.BindIndexBuffer(m_Buffers[pass.indexBuffer].buffer, 0, VK_INDEX_TYPE_UINT32);
            gfxCmd.BindDescriptorSet(m_BoundPipline->GetPipelineLayout(), m_DescriptorSets[m_SwapchainImageIndex]);
            gfxCmd.DrawIndexed(pass.size, 1, 0, 0, 0);
            gfxCmd.EndRenderPass();
        }
    }
    gfxCmd.End();

    std::array cmds = {gfxCmd.GetHandle()};
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    // Graphics queue submit
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_ImageAvailableSems[m_CurrentFrame];
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = static_cast<uint32_t>(cmds.size());
    submitInfo.pCommandBuffers = cmds.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_RenderFinishedSems[m_CurrentFrame];

    m_Device->GetGraphicsQueue()->Submit(submitInfo, m_Fences[m_CurrentFrame].GetHandle());
    
    SubmitFrame();

    m_CurrentFrame = (m_CurrentFrame + 1) % m_SwapchainImageCount;

    FrameMark;
}

RHIResourceIdx VulkanRHI::CreateShader(ShaderDescription description)
{
    VkShaderStageFlagBits stage;
    switch (description.stage) {
        case ShaderStage::Vertex:
            stage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case ShaderStage::Fragment:
            stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        case ShaderStage::Compute:
            stage = VK_SHADER_STAGE_COMPUTE_BIT;
            break;
    }
    m_ShaderModules.push_back(m_Device->CreateShaderModule(std::string(description.file), stage, description.entry));
    return m_ShaderModules.size() - 1;
}

RHIResource VulkanRHI::CreatePipeline(const std::vector<RHIResourceIdx>& shaders)
{
    std::vector<VulkanShaderModule> shaderModules;
    for (RHIResourceIdx shaderIdx : shaders) {
        shaderModules.push_back(m_ShaderModules[shaderIdx]);
    }
    return new VulkanPipeline(m_Device.get(), shaderModules, m_RenderPass, m_Swapchain);
}
RHIResourceIdx VulkanRHI::CreateBuffer(BufferDescription description)
{
    m_BufferDescriptions.push_back(description);
    m_Buffers.emplace_back(VulkanBuffer {});
    return m_Buffers.size() - 1;
}

void VulkanRHI::BindPipeline(RHIResource pipeline)
{
    m_BoundPipline = (VulkanPipeline*)pipeline;
}

void VulkanRHI::DestroyPipeline(RHIResource pipeline)
{
    ((VulkanPipeline*)pipeline)->Destroy();
}

void VulkanRHI::SetPasses(const std::vector<RenderPassDescription>& descriptions)
{
    m_PassDescriptions = descriptions;
}

void VulkanRHI::SetClearColor(float r, float g, float b, float a)
{
    m_ClearValues[0].color = {{r, g, b, a}};
}

void VulkanRHI::SetClearDepth(float depth)
{
    m_ClearValues[1].depthStencil = {depth, 0};
}

void VulkanRHI::CreateInstance()
{
    /// Instance extensions
    uint32_t instanceExtensionCount = 0;    
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
    std::vector<VkExtensionProperties> supportedInstanceExtensions(instanceExtensionCount);
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, supportedInstanceExtensions.data()));
    std::vector<const char*> requiredInstanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface"
    };
    if (m_Settings.validation) {
        requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    if (!validateExtension(requiredInstanceExtensions, supportedInstanceExtensions)) {
        SEFatal("Required extensions not found");
    }

    /// Instance Layers
    uint32_t instanceLayerCount = 0;
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
    std::vector<VkLayerProperties> supportedInstanceLayers(instanceLayerCount);
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedInstanceLayers.data()));
    std::vector<const char*> requiredInstanceLayers;
    if (m_Settings.validation) {
        auto validationLayer = {"VK_LAYER_KHRONOS_validation"};
        requiredInstanceLayers.insert(requiredInstanceLayers.end(), validationLayer.begin(), validationLayer.end());
    }
    if (!validateLayers(requiredInstanceLayers, supportedInstanceLayers)) {
        SEFatal("Required layers not found");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "serious";
    appInfo.pEngineName = "serious";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
    instanceInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size());
    instanceInfo.ppEnabledLayerNames = requiredInstanceLayers.data();
    VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, &m_Instance));

    /// Debug messenger
    if (m_Settings.validation) {
        VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo = {};
        debugUtilsMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugUtilsMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugUtilsMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugUtilsMessengerInfo.pfnUserCallback = VulkanDebugUtilsMessengerCallback;
        m_DebugUtilsMessenger = CreateDebugUtilsMessengerEXT(m_Instance, debugUtilsMessengerInfo);
    }
}

void VulkanRHI::CreateCommandPool()
{
    m_GfxCmdPool = m_Device->CreateCommandPool(*m_Device->GetGraphicsQueue());
    m_TsfCmdPool = m_Device->CreateCommandPool(*m_Device->GetTransferQueue());
    m_GfxCmdBufs.resize(m_SwapchainImageCount);
    
    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i) {
        m_GfxCmdBufs[i] = m_GfxCmdPool.Allocate();
    }
}

void VulkanRHI::CreateSyncObjects()
{
    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i) {
        m_Fences.push_back(m_Device->CreateFence(VK_FENCE_CREATE_SIGNALED_BIT));
        m_ImageAvailableSems.push_back(m_Device->CreateSemaphore());
        m_RenderFinishedSems.push_back(m_Device->CreateSemaphore());
    }
}

void VulkanRHI::CreateRenderPass()
{
    // Color attachment
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format = m_Swapchain.GetColorFormat();
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
    depthAttachment.format = m_Swapchain.GetDepthFormat();
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

    VK_CHECK_RESULT(vkCreateRenderPass(m_Device->GetHandle(), &passInfo, nullptr, &m_RenderPass));
}

void VulkanRHI::CreateFramebuffers()
{
    VkExtent2D extent = m_Swapchain.GetExtent();
    VkImageView depthImageView = m_DepthImage.imageView;
    m_Framebuffers.resize(m_SwapchainImageCount, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i) {
        std::vector<VkImageView> attachments = {
            m_Swapchain.GetImageView(i),
            depthImageView,
        };
        m_Framebuffers[i] = m_Device->CreateFramebuffer(extent, m_RenderPass, attachments);
    }
}

void VulkanRHI::SetDescriptorResources()
{    
    VkDeviceSize uboSize = sizeof(UniformBufferObject);
    m_UniformBuffers.resize(m_SwapchainImageCount, {});
    m_UniformBufferMapped.resize(m_SwapchainImageCount, nullptr);    
    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i) {
        VulkanBuffer& uniformBuffer = m_UniformBuffers[i];
        m_Device->CreateBuffer(
            uniformBuffer,
            uboSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );
        m_Device->MapBuffer(uniformBuffer, uboSize, 0);
        m_UniformBufferMapped[i] = uniformBuffer.mapped;
    }

    auto gfxCmd = m_GfxCmdPool.Allocate();
    m_Device->CreateTextureImage(
        m_TextureImage,
        "D:/w6rsty/dev/Cpp/serious/assets/viking_room.png",
        m_Swapchain.GetColorFormat(),
        m_Swapchain.GetComponentMapping(),
        gfxCmd
    );
    m_GfxCmdPool.Free(gfxCmd);

    VkDescriptorSetLayoutBinding uboLayoutBinding {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding {};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    m_DescriptorSets.resize(m_SwapchainImageCount, VK_NULL_HANDLE);
    m_Device->SetDescriptorSetLayout({
        uboLayoutBinding,
        samplerLayoutBinding
    });

    m_Device->SetDescriptorPool(
        {
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_SwapchainImageCount},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_SwapchainImageCount}
        },
        m_SwapchainImageCount
    );

    m_DescriptorSets.resize(m_SwapchainImageCount);
    m_Device->AllocateDescriptorSets(m_DescriptorSets);

    for (uint32_t i = 0; i < m_SwapchainImageCount; ++i) {
        VkDescriptorBufferInfo bufferInfo {};
        bufferInfo.buffer = m_UniformBuffers[i].buffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = m_TextureImage.imageView;
        imageInfo.sampler = m_TextureImage.sampler;

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

void VulkanRHI::UpdateUniforms()
{
    ZoneScoped;
    UniformBufferObject ubo = {};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.view = m_Camera.matrices.view;
    ubo.proj = m_Camera.matrices.projection;
    memcpy(m_UniformBufferMapped[m_CurrentFrame], &ubo, sizeof(UniformBufferObject));
}

}