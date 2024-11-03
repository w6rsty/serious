#pragma once
#include "serious/graphics/vulkan/VulkanUtils.hpp"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace serious
{

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/**
 * @brief VkFence wrapper class
 *
 * Created and Destroyed through VulkanDevice 
 */
class VulkanFence final
{
public:
    VulkanFence() = default;
    VulkanFence(VkDevice device, VkFenceCreateFlags flags = 0)
    : m_Fence(VK_NULL_HANDLE)
    , m_Device(device)
    {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = flags;
        VK_CHECK_RESULT(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_Fence));
    }
    inline void Wait() const { VK_CHECK_RESULT(vkWaitForFences(m_Device, 1, &m_Fence, VK_TRUE, UINT64_MAX)); }
    inline void Reset() const { VK_CHECK_RESULT(vkResetFences(m_Device, 1, &m_Fence)); }
    inline void WaitAndReset() const { Wait(); Reset(); }
    inline VkFence GetHandle() const { return m_Fence; }
private:
    VkFence  m_Fence;
    VkDevice m_Device;

    friend class VulkanDevice;
};

/**
 * @brief VkShaderModule wrapper
 * 
 */
struct VulkanShaderModule
{
    VkShaderStageFlagBits stage;
    VkShaderModule handle;
    std::string_view entry;
};

/**
 * @brief Buffer related dresource, created by VulkanDevice
 * 
 */
struct VulkanBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped = nullptr;
};

struct VulkanImage
{
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};

/**
 * @brief Texture resource for texture and depth image
 * 
 */
struct VulkanTexture
{
    uint32_t width = 0;
    uint32_t height = 0;
    VulkanImage image = {};
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
};

enum class VulkanQueueUsage
{
    Graphics,
    Compute,
    Transfer,
    Present
};

constexpr const char* VulkanQueueUsageString(VulkanQueueUsage usage)
{
    switch (usage) {
        case VulkanQueueUsage::Graphics: return "Graphics";
        case VulkanQueueUsage::Compute: return "Compute";
        case VulkanQueueUsage::Transfer: return "Transfer";
        case VulkanQueueUsage::Present: return "Present";
    }
}

}