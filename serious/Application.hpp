#pragma once

#include "serious/Renderer.hpp"
#include "serious/VulkanSwapchain.hpp"
#include "serious/VulkanContext.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class Application
{
public:
    static void Init(
        SurfaceCreateFunc&& surfaceCreateFunc,
        uint32_t width,
        uint32_t height,
        bool vsync,
        GetWindowSizeFunc&& getWindowSizeFunc);
    static void Shutdown();
    static void OnUpdate();

    static inline Ref<Renderer> s_Renderer = nullptr;
};

}
