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

#define Info(...) \
fmt::print(fmt::fg(fmt::color::teal), __VA_ARGS__); \
fmt::print("\n")

#define Warn(...) \
fmt::print(fmt::fg(fmt::color::peru), __VA_ARGS__); \
fmt::print("\n")

#define Error(...) \
fmt::print(fmt::fg(fmt::color::red), __VA_ARGS__); \
fmt::print("\n")

#if defined(DEBUG)
    #define Fatal(...) \
    fmt::print(fmt::fg(fmt::color::red), __VA_ARGS__); \
    fmt::print("\n"); \
    __debugbreak(); \
    std::terminate()
#else
    #define Fatal(...) \
    fmt::print(fmt::fg(fmt::color::red), __VA_ARGS__); \
    fmt::print("\n"); \
    std::terminate()
#endif

#if defined(_DEBUG) || defined(ENABLE_VALIDATION)
    static constexpr bool s_Validation = true;
#else
    static constexpr bool s_Validation = false;
#endif


#if defined(_DEBUG)
#define VK_CHECK_RESULT(result) \
if (result != VK_SUCCESS) { \
    Fatal("Vulkan error"); \
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
            Error("Required extension {} not found", extension);
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
            Error("Required layer {} not found", layer);
            return false;
        }
    }
    return true;
}

constexpr const char* VulkanDebugUtilsMessageSeverity(const VkDebugUtilsMessageSeverityFlagBitsEXT severity)
{
    switch (severity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:   return "VERBOSE";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:      return "INFO";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:   return "WARNING";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:     return "ERROR";
        default:                                                return "UNKNOWN";
    }
}
   
static inline VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    (void)pUserData;
    (void)messageType;
    Warn("[{}] {}", VulkanDebugUtilsMessageSeverity(messageSeverity), pCallbackData->pMessage);
    return VK_FALSE;
}

static inline VkDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT& creaetInfo)
{
    static PFN_vkCreateDebugUtilsMessengerEXT fpCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    VkDebugUtilsMessengerEXT debugUtilsMessenger;
    VK_CHECK_RESULT(fpCreateDebugUtilsMessengerEXT(instance, &creaetInfo, nullptr, &debugUtilsMessenger));
    return debugUtilsMessenger;
}

static inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugUtilsMessenger)
{
    static PFN_vkDestroyDebugUtilsMessengerEXT fpDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (debugUtilsMessenger != VK_NULL_HANDLE) {
        fpDestroyDebugUtilsMessengerEXT(instance, debugUtilsMessenger, nullptr);
    }
}

constexpr const char* VulkanPresentModeString(const VkPresentModeKHR presentMode)
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

constexpr const char* VulkanFormatString(const VkFormat format)
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

constexpr const char* VulkanColorSpaceString(const VkColorSpaceKHR colorSpace)
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

}
