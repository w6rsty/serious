#include "serious/vulkan/VulkanDevice.hpp"

namespace serious
{

VulkanQueue::VulkanQueue(VulkanDevice* device, uint32_t familyIndex)
    : m_Queue(VK_NULL_HANDLE)
    , m_QueueIndex(0)
    , m_FamilyIndex(familyIndex)
    , m_Device(device)
{
    vkGetDeviceQueue(m_Device->GetHandle(), m_FamilyIndex, m_QueueIndex, &m_Queue);
}

VulkanQueue::~VulkanQueue()
{
}

void VulkanQueue::Submit(const VkSubmitInfo& submitInfo, VkFence fence)
{
    VK_CHECK_RESULT(vkQueueSubmit(m_Queue, 1, &submitInfo, fence));
}

void VulkanQueue::WaitIdle()
{
    VK_CHECK_RESULT(vkQueueWaitIdle(m_Queue));
}

VulkanDevice::VulkanDevice(VkInstance instance)
    : m_Device(VK_NULL_HANDLE)
    , m_Gpu(VK_NULL_HANDLE)
    , m_GraphicsQueue(nullptr)
    , m_ComputeQueue(nullptr)
    , m_TransferQueue(nullptr)
    , m_PresentQueue(nullptr)
{
    SelectGpu(instance);

    uint32_t deviceExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_Gpu, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> supportedDeviceExtensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(m_Gpu, nullptr, &deviceExtensionCount, supportedDeviceExtensions.data());
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME  
    };
    if (!validateExtension(deviceExtensions, supportedDeviceExtensions)) {
        Fatal("required device extensions not found");
    }

    /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkQueueFamilyProperties.html
    /// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceQueueCreateInfo.html
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_Gpu, &queueFamilyCount, queueFamilyProperties.data());
    Info("max anisotropy: {}", m_GpuProps.limits.maxSamplerAnisotropy);
    Info("found {} available queue(s)", queueFamilyCount);
    
    /// Queues create infos(Reference from Unreal Engine VulkanRHI)
    std::vector<VkDeviceQueueCreateInfo> queueFamilyInfos;
    uint32_t numPriorities = 0;
    int32_t graphicsQueueFamilyIndex = -1;
    int32_t computeQueueFamilyIndex  = -1;
    int32_t transferQueueFamilyIndex = -1;

    constexpr VkFlags requiredFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
    VkFlags dispatchedQueues = 0;

    for (int32_t familyIndex = 0; familyIndex < static_cast<int32_t>(queueFamilyCount); ++familyIndex) {
        const VkQueueFamilyProperties& currProps = queueFamilyProperties[static_cast<size_t>(familyIndex)];

        bool isValidQueue = false;
        if ((currProps.queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
            (graphicsQueueFamilyIndex == -1)) {
            graphicsQueueFamilyIndex = familyIndex;
            isValidQueue = true;
            dispatchedQueues |= VK_QUEUE_GRAPHICS_BIT;
        }
        if ((currProps.queueFlags & VK_QUEUE_COMPUTE_BIT) &&
            (computeQueueFamilyIndex == -1) &&
            (graphicsQueueFamilyIndex != familyIndex)) {
            computeQueueFamilyIndex = familyIndex;
            isValidQueue = true;
            dispatchedQueues |= VK_QUEUE_COMPUTE_BIT;
        }
        if ((currProps.queueFlags & VK_QUEUE_TRANSFER_BIT) &&
            (transferQueueFamilyIndex == -1) &&
            (graphicsQueueFamilyIndex != familyIndex) &&
            (computeQueueFamilyIndex  != familyIndex)) {
            transferQueueFamilyIndex = familyIndex;
            isValidQueue = true;
            dispatchedQueues |= VK_QUEUE_TRANSFER_BIT;
        }
        if (!isValidQueue) {
            Warn("skipped queue family {}: {} queues", familyIndex, currProps.queueCount);
            continue;
        }

        size_t queueIndex = queueFamilyInfos.size();
        queueFamilyInfos.emplace_back();
        VkDeviceQueueCreateInfo& currQueue = queueFamilyInfos[queueIndex];
        currQueue.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        currQueue.queueCount = currProps.queueCount;
        currQueue.queueFamilyIndex = static_cast<uint32_t>(familyIndex);
        numPriorities += currProps.queueCount;
        Info("use queue family {}: {} queues", familyIndex, currProps.queueCount);

        if ((dispatchedQueues & requiredFlags) == requiredFlags) {
            break;
        }
    }

    std::vector<float> queuePriorities(numPriorities);
    float* currPriority = queuePriorities.data();
    for (uint32_t infoIndex = 0; infoIndex < queueFamilyInfos.size(); ++infoIndex) {
        VkDeviceQueueCreateInfo& currQueue = queueFamilyInfos[infoIndex];
        currQueue.pQueuePriorities = currPriority;

        const VkQueueFamilyProperties& currProps = queueFamilyProperties[currQueue.queueFamilyIndex];
        for (uint32_t queueIndex = 0; queueIndex < currProps.queueCount; ++queueIndex) {
            *currPriority++ = 1.0f;
        }
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.queueCreateInfoCount = queueFamilyInfos.size();
    deviceInfo.pQueueCreateInfos = queueFamilyInfos.data();
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    VK_CHECK_RESULT(vkCreateDevice(m_Gpu, &deviceInfo, nullptr, &m_Device));

    /// Create queues https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetDeviceQueue.html
    m_GraphicsQueue = CreateRef<VulkanQueue>(this, graphicsQueueFamilyIndex);
    if (computeQueueFamilyIndex == -1) {
        computeQueueFamilyIndex = graphicsQueueFamilyIndex;
    }
    m_ComputeQueue = CreateRef<VulkanQueue>(this, computeQueueFamilyIndex);
    if (transferQueueFamilyIndex == -1) {
        transferQueueFamilyIndex = computeQueueFamilyIndex;
    }
    m_TransferQueue = CreateRef<VulkanQueue>(this, transferQueueFamilyIndex);
}

VulkanDevice::~VulkanDevice()
{
}

void VulkanDevice::Destroy()
{
    vkDestroyDevice(m_Device, nullptr);
}

void VulkanDevice::SetupPresentQueue(VkSurfaceKHR surface)
{
    const auto isSupportPresent = [surface](VkPhysicalDevice gpu, Ref<VulkanQueue> queue) {
        VkBool32 result = VK_FALSE;
        uint32_t familyIndex = queue->GetFamilyIndex();
        VK_CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu, familyIndex, surface, &result));
        return result == VK_TRUE;
    };
    if (!isSupportPresent(m_Gpu, m_GraphicsQueue)) {
        Fatal("failed to find a queue family that supports presentation");
    }
    if (isSupportPresent(m_Gpu, m_ComputeQueue)) {
        m_PresentQueue = m_ComputeQueue;
    } else {
        m_PresentQueue = m_GraphicsQueue;
    }
    Info("use queue family {} for presentation", m_PresentQueue->GetFamilyIndex());
}

void VulkanDevice::SelectGpu(VkInstance instance)
{
    /// Don't use VulkanInstance::Get() here, because this function is still in the middle of
    /// context's construction, but VkInstance has been created already
    uint32_t physicalDeviceCount = 0;
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));
    if (physicalDeviceCount == 0) { Fatal("No device support Vulkan found"); }
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data()));

    bool foundSuitableGpu = false;
    for (const auto& gpu : physicalDevices) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(gpu, &properties);
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_Gpu = gpu;
            m_GpuProps = properties;
            foundSuitableGpu = true;
            break;
        }
    }

    if (!foundSuitableGpu) {
        Fatal("No suitable physical device");
    }

    Info("using device: {}", m_GpuProps.deviceName);
}

}
