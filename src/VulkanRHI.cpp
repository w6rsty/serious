#include "serious/VulkanRHI.hpp"
#include "serious/vulkan/VulkanContext.hpp"
#include "serious/vulkan/VulkanWindow.hpp"
#include "serious/vulkan/VulkanDevice.hpp"
#include "serious/vulkan/VulkanSwapchain.hpp"
#include "serious/vulkan/VulkanRenderer.hpp"

namespace serious
{

VulkanRHI::VulkanRHI()
    : m_Instance(nullptr)
    , m_Window(nullptr)
    , m_Device(nullptr)
    , m_Swapchain(nullptr)
    , m_Renderer(nullptr)
{
}

void VulkanRHI::Init(SurfaceCreateFunc&& surfaceCreateFunc, GetWindowSpecFunc&& getWindowSizeFunc)
{
    m_Instance = CreateRef<VulkanInstance>();
    m_Window = CreateRef<VulkanWindow>(m_Instance->GetHandle(), surfaceCreateFunc, getWindowSizeFunc);
    m_Device = CreateRef<VulkanDevice>(m_Instance->GetHandle());
    m_Device->SetupPresentQueue(m_Window->GetSurfaceHandle());
    m_Swapchain = CreateRef<VulkanSwapchain>(m_Device.get(), m_Window.get());
    m_Swapchain->Create(nullptr);
    m_Renderer = CreateRef<VulkanRenderer>(m_Device.get(), m_Swapchain.get());

}

void VulkanRHI::Shutdown()
{
    m_Renderer->Destroy();
    m_Swapchain->Destroy();
    m_Device->Destroy();
    m_Window->Destroy();
    m_Instance->Destroy();
}

void VulkanRHI::Update()
{
    m_Renderer->OnUpdate();
}

}