#pragma once

#include "serious/VulkanUtils.hpp"
#include "serious/vulkan/VulkanObjects.hpp"

#include <vulkan/vulkan.h>

namespace serious
{

class VulkanCommandPool;
class VulkanCommandBuffer;

class VulkanQueue final
{
public:
    VulkanQueue(VulkanDevice* device, uint32_t familyIndex);
    ~VulkanQueue();

    void Submit(const VkSubmitInfo& submitInfo, VkFence fence = VK_NULL_HANDLE);
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
    void SetPresentQueue(VkSurfaceKHR surface);
    void WaitIdle();

    VulkanShaderModule CreateShaderModule(std::string_view file, VkShaderStageFlagBits flag, std::string_view entry);
    VulkanFence        CreateFence(VkFenceCreateFlags flags = 0);
    VkSemaphore        CreateSemaphore();
    VulkanImage        CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling imageTiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
    VkImageView        CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkComponentMapping mapping = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY});
    VkFramebuffer      CreateFramebuffer(const VkExtent2D& extent, VkRenderPass renderPass, const std::vector<VkImageView>& attachments);
    VulkanCommandPool  CreateCommandPool(const VulkanQueue& queue);
    void               CreateBuffer(VulkanBuffer& buffer, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    void               CopyToBuffer(VulkanBuffer& buffer, const void* data, VkDeviceSize size);
    void               CopyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size, VkDeviceSize offset, VulkanCommandBuffer& tsfCmd);
    void               CreateDeviceBuffer(VulkanBuffer& buffer, VkDeviceSize size, void* data, VkBufferUsageFlags usage, VulkanCommandBuffer& tsfCmd);
    void               MapBuffer(VulkanBuffer& buffer, VkDeviceSize size, VkDeviceSize offset);
    void               UnmapBuffer(VulkanBuffer& buffer);
    void               TransitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, VkImageAspectFlags aspectFlags, VulkanCommandBuffer& cmd);
    void               CreateTextureImage(VulkanTexture& texture, const std::string& path, VkFormat format, VkComponentMapping mapping, VulkanCommandBuffer& gfxCmd);
    void               CreateDepthImage(VulkanTexture& texture, const VkExtent2D& extent, VulkanCommandBuffer& gfxCmd);

    void DestroyImage(VulkanImage& image);
    void DestroyShaderModule(VulkanShaderModule& shaderModule);
    void DestroyFence(VulkanFence& fence);
    void DestroyCommandPool(VulkanCommandPool& cmdPool);
    // Destroy buffer created by CreateBuffer and CreateDeviceBuffer
    void DestroyBuffer(VulkanBuffer& buffer);
    // Destroy texture image created by CreateTextureImage and CreateDepthImage
    void DestroyTextureImage(VulkanTexture& texture);
    
    void SetDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
    void SetDescriptorPool(const std::vector<VkDescriptorPoolSize>& poolSizes, uint32_t maxSets);
    void AllocateDescriptorSets(std::vector<VkDescriptorSet>& descriptorSets);
    void DestroyDescriptorResources();

    inline VkDevice                   GetHandle() const { return m_Device; } 
    inline VkPhysicalDevice           GetGpuHandle() const { return m_Gpu; }
    inline VkPhysicalDeviceProperties GetGpuProperties() const { return m_GpuProps; }
    inline VkDescriptorSetLayout      GetDescriptorSetLayout() const { return m_DescriptorSetLayout; }
    inline Ref<VulkanQueue>           GetGraphicsQueue() { return m_GraphicsQueue; }
    inline Ref<VulkanQueue>           GetComputeQueue() { return m_ComputeQueue; }
    inline Ref<VulkanQueue>           GetTransferQueue() { return m_TransferQueue; }
    inline Ref<VulkanQueue>           GetPresentQueue() { return m_PresentQueue; }
private:
    void SelectGpu(VkInstance instance);
    uint32_t FindMemoryTypeIdx(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);
private:
    VkDevice m_Device;
    VkPhysicalDevice m_Gpu;
    VkPhysicalDeviceProperties m_GpuProps;
    VkPhysicalDeviceMemoryProperties m_GpuMemoryProps;
    bool m_DeviceLocalMemorySupport;
    VulkanFence m_OperationFence;
    
    Ref<VulkanQueue> m_GraphicsQueue;
    Ref<VulkanQueue> m_ComputeQueue;
    Ref<VulkanQueue> m_TransferQueue;
    Ref<VulkanQueue> m_PresentQueue;

    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkDescriptorPool m_DescriptorPool;
};

}
