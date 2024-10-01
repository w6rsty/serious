#pragma once

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanInstance final
{
public:
    VulkanInstance();
    ~VulkanInstance();
    void Destroy();
    
    inline VkInstance GetHandle() { return m_Instance; }
private:
    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT m_DebugUtilsMessenger;
};

}
