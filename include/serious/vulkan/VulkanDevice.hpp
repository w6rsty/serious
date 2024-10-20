#pragma once

#include "serious/VulkanUtils.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanCommandPool;

class VulkanDevice;

class VulkanQueue final
{
public:
    VulkanQueue(VulkanDevice* device, uint32_t familyIndex);
    ~VulkanQueue();

    void Submit(const VkSubmitInfo& submitInfo, VkFence fence);
    void WaitIdle();

    inline VkQueue GetHandle() const { return m_Queue; }
    inline uint32_t GetHandleIndex() const { return m_QueueIndex; }
    inline uint32_t GetFamilyIndex() const { return m_FamilyIndex; }
private:
    VkQueue m_Queue;
    uint32_t m_QueueIndex;
    uint32_t m_FamilyIndex;
    VulkanDevice* m_Device;
};

class VulkanDevice final
{
public:
    VulkanDevice(VkInstance instance);
    ~VulkanDevice();

    void Destroy();
    void SetupPresentQueue(VkSurfaceKHR surface);

    inline VkDevice                   GetHandle() const { return m_Device; } 
    inline VkPhysicalDevice           GetGpuHandle() const { return m_Gpu; }
    inline VkPhysicalDeviceProperties GetGpuProperties() const { return m_GpuProps; }
    inline Ref<VulkanQueue>           GetGraphicsQueue() { return m_GraphicsQueue; }
    inline Ref<VulkanQueue>           GetComputeQueue() { return m_ComputeQueue; }
    inline Ref<VulkanQueue>           GetTransferQueue() { return m_TransferQueue; }
    inline Ref<VulkanQueue>           GetPresentQueue() { return m_PresentQueue; }
private:
    void SelectGpu(VkInstance instance);
    Ref<VulkanCommandPool> GetOrCreateThreadLocalCommandPool();
private:
    VkDevice m_Device;
    VkPhysicalDevice m_Gpu;
    VkPhysicalDeviceProperties m_GpuProps;

    Ref<VulkanQueue> m_GraphicsQueue;
    Ref<VulkanQueue> m_ComputeQueue;
    Ref<VulkanQueue> m_TransferQueue;
    Ref<VulkanQueue> m_PresentQueue;
};

}