#include "serious/vulkan/VulkanWindow.hpp"

namespace serious
{

VulkanWindow::VulkanWindow(VkInstance instance, SurfaceCreateFunc surfaceCreateFunc, GetWindowSpecFunc windowSpecFunc)
    : m_Surface(VK_NULL_HANDLE)
    , m_Instance(instance)
    , m_GetWindowSpecFunc(windowSpecFunc)
{
    m_Surface = static_cast<VkSurfaceKHR>(surfaceCreateFunc(instance));
}

VulkanWindow::~VulkanWindow()
{
}

void VulkanWindow::Destroy()
{
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
}

}