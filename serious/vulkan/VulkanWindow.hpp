#pragma once

#include "serious/RHI.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanWindow final
{
public:
    VulkanWindow(VkInstance instance, SurfaceCreateFunc surfaceCreateFunc, GetWindowSpecFunc windowSpecFunc);
    ~VulkanWindow();
    void Destroy();

    inline WindowSpec   GetWindowSpec() const { return m_GetWindowSpecFunc(); }    
    inline VkSurfaceKHR GetSurfaceHandle() const { return m_Surface; }
private:
    VkSurfaceKHR m_Surface;
    VkInstance m_Instance;
    GetWindowSpecFunc m_GetWindowSpecFunc;
};

}