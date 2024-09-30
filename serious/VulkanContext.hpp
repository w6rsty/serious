#pragma once

#include "serious/VulkanDevice.hpp"
#include "serious/VulkanSwapchain.hpp"

#include <vulkan/vulkan.h>

#include <memory>
#include <functional>

namespace serious
{

using SurfaceCreateFunc = std::function<VkSurfaceKHR(VkInstance)>;

class VulkanContext final
{
public:
    ~VulkanContext();

    inline static VulkanContext& Get() { return *m_Instance; } 
    static VulkanContext& Init(SurfaceCreateFunc&& surfaceCreateFunc, uint32_t width, uint32_t height, bool vsync);
    static void Shutdown();
    static inline VkInstance GetVulkanInstance() { return m_VulkanInstance; }
    
    inline Ref<VulkanDevice> GetDevice() { return m_Device; }
    inline VkDevice GetDeviceHandle() { return m_Device->GetHandle(); }
    inline VkPhysicalDevice GetGpuHandle() { return m_Device->GetGpuHandle(); }
    inline VkSurfaceKHR GetSurfaceHandle() { return m_Surface; }
    inline Ref<VulkanSwapchain> GetSwapchain() { return m_Swapchain; }
private:
    VulkanContext();
    void InitDevice();
    void InitSurface(SurfaceCreateFunc&& surfaceCreateFunc);
    void InitSwapchain(uint32_t width, uint32_t height, bool vsync);
private:
    static inline VkInstance m_VulkanInstance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
    Ref<VulkanDevice> m_Device;
    VkSurfaceKHR m_Surface;
    Ref<VulkanSwapchain> m_Swapchain;

    static inline VulkanContext* m_Instance = nullptr;
};

}
