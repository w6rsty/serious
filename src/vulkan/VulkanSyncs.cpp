#include "serious/vulkan/VulkanSyncs.hpp"
#include "serious/vulkan/VulkanDevice.hpp"

namespace serious
{

VulkanSemaphore::VulkanSemaphore(VulkanDevice* device)
    : m_Semaphore(VK_NULL_HANDLE)
    , m_Device(device)
{
    VkSemaphoreCreateInfo semaInfo = {};
    semaInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK_RESULT(vkCreateSemaphore(device->GetHandle(), &semaInfo, nullptr, &m_Semaphore));
}

VulkanSemaphore::~VulkanSemaphore()
{
}

void VulkanSemaphore::Destroy()
{
    vkDestroySemaphore(m_Device->GetHandle(), m_Semaphore, nullptr);
}

VulkanFence::VulkanFence(VulkanDevice* device, VkFenceCreateFlags flags)
    : m_Fence(VK_NULL_HANDLE)
    , m_Device(device)
{
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    VK_CHECK_RESULT(vkCreateFence(m_Device->GetHandle(), &fenceInfo, nullptr, &m_Fence));
}

VulkanFence::~VulkanFence()
{
}

void VulkanFence::Destroy()
{
    vkDestroyFence(m_Device->GetHandle(), m_Fence, nullptr);
}

void VulkanFence::Wait() const
{
    VK_CHECK_RESULT(vkWaitForFences(m_Device->GetHandle(), 1, &m_Fence, VK_FALSE, UINT64_MAX));
}

void VulkanFence::Reset() const
{
    VK_CHECK_RESULT(vkResetFences(m_Device->GetHandle(), 1, &m_Fence));
}

void VulkanFence::WaitAndReset() const
{
    Wait();
    Reset();
}

bool VulkanFence::GetStatus() const
{
    VkResult result = vkGetFenceStatus(m_Device->GetHandle(), m_Fence);
    return result == VK_SUCCESS;
}

}