#pragma once

#include <vulkan/vulkan.h>

#include <fmt/core.h>
#include <fmt/color.h>

#include <string>
#include <vector>
#include <memory>

namespace serious
{

template <class T>
using Ref = std::shared_ptr<T>;

template <class T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

std::string ReadFile(const std::string& filename);

#define VKInfo(...) \
fmt::print(fmt::fg(fmt::color::teal), __VA_ARGS__); \
fmt::print("\n")

#define VKWarn(...) \
fmt::print(fmt::fg(fmt::color::peru), __VA_ARGS__); \
fmt::print("\n")

#define VKError(...) \
fmt::print(fmt::fg(fmt::color::red), __VA_ARGS__); \
fmt::print("\n")

#define VKFatal(...) \
fmt::print(fmt::fg(fmt::color::red), __VA_ARGS__); \
fmt::print(fmt::fg(fmt::color::red), " At {} {} {}\n", __FILE__, __LINE__, __FUNCTION__); \
fmt::print("\n"); \
std::terminate()

#if defined(_DEBUG)
#define VK_CHECK_RESULT(result) \
if (result != VK_SUCCESS) { \
    VKError("Vulkan error : {:x}", (uint32_t)result); \
}
#else
#define VK_CHECK_RESULT(result) result
#endif

/// From https://github.com/KhronosGroup/Vulkan-Samples
static inline bool validateExtension(const std::vector<const char*>& required, const std::vector<VkExtensionProperties>& available)
{
    for (const auto& extension : required) {
        if (std::find_if(
            available.begin(),
            available.end(),
            [&extension](const VkExtensionProperties& ep) {
                return strcmp(ep.extensionName, extension) == 0;
            }) == available.end()
        ) {
            VKError("Required extension {} not found", extension);
            return false;
        }
    }
    return true;
}

/// From https://github.com/KhronosGroup/Vulkan-Samples
static inline bool validateLayers(const std::vector<const char*>& required, const std::vector<VkLayerProperties>& available)
{
    for (const auto& layer : required) {
        if (std::find_if(
            available.begin(),
            available.end(),
            [&layer](const VkLayerProperties& lp) {
                return strcmp(lp.layerName, layer) == 0;
            }) == available.end()
        ) {
            VKError("Required layer {} not found", layer);
            return false;
        }
    }
    return true;
}

constexpr const char* VulkanPresentModeString(VkPresentModeKHR presentMode)
{
    switch (presentMode) {
        case VK_PRESENT_MODE_IMMEDIATE_KHR: return "VK_PRESENT_MODE_IMMEDIATE_KHR";
        case VK_PRESENT_MODE_MAILBOX_KHR: return "VK_PRESENT_MODE_MAILBOX_KHR";
        case VK_PRESENT_MODE_FIFO_KHR: return "VK_PRESENT_MODE_FIFO_KHR";
        case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "VK_PRESENT_MODE_FIFO_RELAXED_KHR";
        default: return "UNKNOWN PRESENT MODE";
          break;
        }
}

constexpr const char* VulkanFormatString(VkFormat format)
{
    switch (format) {
        case VK_FORMAT_UNDEFINED: return "VK_FORMAT_UNDEFINED";
        case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
        case VK_FORMAT_B8G8R8A8_UNORM: return "VK_FORMAT_B8G8R8A8_UNORM";
        case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
        case VK_FORMAT_B8G8R8A8_SRGB: return "VK_FORMAT_B8G8R8A8_SRGB";
        case VK_FORMAT_R8G8B8A8_SNORM: return "VK_FORMAT_R8G8B8A8_SNORM";
        case VK_FORMAT_B8G8R8A8_SNORM: return "VK_FORMAT_B8G8R8A8_SNORM";
        case VK_FORMAT_R8G8B8A8_UINT: return "VK_FORMAT_R8G8B8A8_UINT";
        case VK_FORMAT_B8G8R8A8_UINT: return "VK_FORMAT_B8G8R8A8_UINT";
        case VK_FORMAT_R8G8B8A8_SINT: return "VK_FORMAT_R8G8B8A8_SINT";
        case VK_FORMAT_B8G8R8A8_SINT: return "VK_FORMAT_B8G8R8A8_SINT";
        default: return "UNKNOWN FORMAT";
    }
}

constexpr const char* VulkanColorSpaceString(VkColorSpaceKHR colorSpace)
{
    switch (colorSpace) {
        case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: return "VK_COLOR_SPACE_SRGB_NONLINEAR_KHR";
        case VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT: return "VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT: return "VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT";
        case VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT: return "VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT";
        case VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT: return "VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT";
        case VK_COLOR_SPACE_BT709_LINEAR_EXT: return "VK_COLOR_SPACE_BT709_LINEAR_EXT";
        default: return "UNKNOWN COLOR SPACE";
    }
}

constexpr const char* VulkanQueueString(uint32_t queueFlag)
{
    switch (queueFlag) {
        case VK_QUEUE_GRAPHICS_BIT: return "graphics";
        case VK_QUEUE_COMPUTE_BIT: return "compute";
        case VK_QUEUE_TRANSFER_BIT: return "transfer";
        case VK_QUEUE_SPARSE_BINDING_BIT: return "sparse binding";
        default: return "unknown";
    }
}

constexpr const char* VulkanMemoryProperty(VkMemoryPropertyFlagBits memoryProperty)
{
    switch (memoryProperty) {
        case VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: return "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT";
        case VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: return "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT";
        case VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: return "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT";
        case VK_MEMORY_PROPERTY_HOST_CACHED_BIT: return "VK_MEMORY_PROPERTY_HOST_CACHED_BIT";
        case VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT: return "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT";
        default: return "UNKNOWN MEMORY PROPERTY";
    }
}

}
