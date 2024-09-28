#include "serious/Application.hpp"
#include "serious/VulkanContext.hpp"

namespace serious
{

void Application::Init(SurfaceCreateFunc&& surfaceCreateFunc, uint32_t width, uint32_t height, bool vsync)
{
    VulkanContext::Init(std::move(surfaceCreateFunc), width, height, vsync);
}

void Application::Shutdown()
{
    VulkanContext::Shutdown();
}

void Application::OnResize(uint32_t width, uint32_t height, bool vsync)
{
    VulkanSwapchainRecreateInfo recreateInfo;
    recreateInfo.oldSwapchain = VulkanContext::Get().GetSwapchain()->GetHandle();
    VulkanContext::Get().GetSwapchain()->Create(width, height, vsync, &recreateInfo);
}

}