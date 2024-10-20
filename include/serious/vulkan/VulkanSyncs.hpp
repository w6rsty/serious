#pragma once

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanDevice;

/// Synchronization primitives wrapper

class VulkanSemaphore final
{
public:
    VulkanSemaphore(VulkanDevice* device);
    ~VulkanSemaphore();
    void Destroy();

    inline VkSemaphore GetHandle() const { return m_Semaphore; }
    inline VkSemaphore* GetHandlePtr() { return &m_Semaphore; }
private:
    VkSemaphore m_Semaphore;
    VulkanDevice* m_Device;
};

class VulkanFence final
{
public:
    VulkanFence(VulkanDevice* device, VkFenceCreateFlags flags = 0);
    ~VulkanFence();
    void Destroy();

    void Wait() const;
    void Reset() const;
    void WaitAndReset() const;
    bool GetStatus() const;

    inline VkFence GetHandle() const { return m_Fence; }
private:
    VkFence m_Fence;
    VulkanDevice* m_Device;
};

}