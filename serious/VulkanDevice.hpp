#pragma once

#include "serious/VulkanUtils.hpp"
#include "serious/VulkanCommand.hpp"

#include <vulkan/vulkan.h>

#include <map>
#include <thread>

namespace serious
{

class VulkanDevice;

class VulkanQueue final
{
public:
    VulkanQueue(VulkanDevice* device, uint32_t familyIndex);
    ~VulkanQueue();

    inline VkQueue GetQueue() const { return m_Queue; }
    inline uint32_t GetQueueIndex() const { return m_QueueIndex; }
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
    VulkanDevice();
    ~VulkanDevice();

    void Destroy();
    void SetupPresentQueue(VkSurfaceKHR surface);
    VkCommandBuffer GetCommandBuffer();

    inline VkDevice GetHandle() const { return m_Device; } 
    inline VkPhysicalDevice GetGpuHandle() const { return m_Gpu; }
    inline Ref<VulkanQueue> GetGraphicsQueue() { return m_GraphicsQueue; }
    inline Ref<VulkanQueue> GetComputeQueue() { return m_ComputeQueue; }
    inline Ref<VulkanQueue> GetTransferQueue() { return m_TransferQueue; }
    inline Ref<VulkanQueue> GetPresentQueue() { return m_PresentQueue; }
private:
    void SelectGpu();
    Ref<VulkanCommandPool> GetOrCreateThreadLocalCommandPool();
private:
    VkDevice m_Device;
    VkPhysicalDevice m_Gpu;
    VkPhysicalDeviceProperties m_GpuProps;

    std::map<std::thread::id, Ref<VulkanCommandPool>> m_CommandPools;

    Ref<VulkanQueue> m_GraphicsQueue;
    Ref<VulkanQueue> m_ComputeQueue;
    Ref<VulkanQueue> m_TransferQueue;
    Ref<VulkanQueue> m_PresentQueue;
};

}
