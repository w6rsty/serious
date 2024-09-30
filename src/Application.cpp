#include "serious/Application.hpp"
#include "serious/VulkanContext.hpp"

namespace serious
{

void Application::Init(SurfaceCreateFunc&& surfaceCreateFunc, uint32_t width, uint32_t height, bool vsync, GetWindowSizeFunc&& getWindowSizeFunc)
{
    VulkanContext& context = VulkanContext::Init(std::move(surfaceCreateFunc), width, height, vsync);
    context.GetSwapchain()->SetGetSize(getWindowSizeFunc);

    s_Renderer = CreateRef<Renderer>(context.GetDevice().get(), context.GetSwapchain().get());
}

void Application::Shutdown()
{
    s_Renderer.reset();
    VulkanContext::Shutdown();
}

void Application::OnUpdate()
{
    s_Renderer->OnUpdate();
}

}