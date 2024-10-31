#pragma once
#include "serious/VulkanUtils.hpp"
#include <vulkan/vulkan.h>

namespace serious
{

/**
 * @brief VkFence wrapper class
 *
 * Created and Destroyed through VulkanDevice 
 */
class VulkanFence final
{
public:
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
    VkShaderModule handle;
    VkShaderStageFlagBits stage;
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

}