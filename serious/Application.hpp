#pragma once

#include "serious/VulkanContext.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class Application
{
public:
    static void Init(SurfaceCreateFunc&& surfaceCreateFunc, uint32_t width, uint32_t height, bool vsync);
    static void Shutdown();
    static void OnResize(uint32_t width, uint32_t height, bool vsync);
};

}
