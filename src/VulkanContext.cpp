#include "serious/VulkanContext.hpp"
#include "serious/VulkanDevice.hpp"

#include <vector>

namespace serious
{

/// From https://github.com/KhronosGroup/Vulkan-Samples
static inline std::vector<const char*> getOptimalValidationLayer(const std::vector<VkLayerProperties> &supported_instance_layers)
{
	static std::vector<std::vector<const char *>> validation_layer_priority_list =
    {
        {"VK_LAYER_KHRONOS_validation"},
        {"VK_LAYER_LUNARG_standard_validation"},
        {
            "VK_LAYER_GOOGLE_threading",
            "VK_LAYER_LUNARG_parameter_validation",
            "VK_LAYER_LUNARG_object_tracker",
            "VK_LAYER_LUNARG_core_validation",
            "VK_LAYER_GOOGLE_unique_objects",
        },
        {"VK_LAYER_LUNARG_core_validation"}
    };
	for (const auto& layer : validation_layer_priority_list)
	{
		if (validateLayers(layer, supported_instance_layers))
		{
			return layer;
		}

		Warn("No supported validation layers");
	}
	return {};
}

VulkanContext::VulkanContext()
    : m_DebugUtilsMessenger(VK_NULL_HANDLE)
    , m_Device(nullptr)
    , m_Surface(VK_NULL_HANDLE)
{
    /// Instance extensions
    uint32_t instanceExtensionCount = 0;    
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
    std::vector<VkExtensionProperties> supportedInstanceExtensions(instanceExtensionCount);
    VK_CHECK_RESULT(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, supportedInstanceExtensions.data()));
    std::vector<const char*> requiredInstanceExtensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        "VK_KHR_win32_surface"
    };
    requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (!validateExtension(requiredInstanceExtensions, supportedInstanceExtensions)) {
        Fatal("Required extensions not found");
    }

    /// Instance Layers
    uint32_t instanceLayerCount = 0;
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
    std::vector<VkLayerProperties> supportedInstanceLayers(instanceLayerCount);
    VK_CHECK_RESULT(vkEnumerateInstanceLayerProperties(&instanceLayerCount, supportedInstanceLayers.data()));
    std::vector<const char*> requiredInstanceLayers;
    if (s_Validation) {
        auto validationLayer = getOptimalValidationLayer(supportedInstanceLayers);
        requiredInstanceLayers.insert(requiredInstanceLayers.end(), validationLayer.begin(), validationLayer.end());
    }
    if (!validateLayers(requiredInstanceLayers, supportedInstanceLayers)) {
        Fatal("Required layers not found");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "serious";
    appInfo.pEngineName = "serious";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
    instanceInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(requiredInstanceLayers.size());
    instanceInfo.ppEnabledLayerNames = requiredInstanceLayers.data();
    VK_CHECK_RESULT(vkCreateInstance(&instanceInfo, nullptr, &m_VulkanInstance));

    /// Debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerInfo = {};
    debugUtilsMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugUtilsMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugUtilsMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    debugUtilsMessengerInfo.pfnUserCallback = VulkanDebugUtilsMessengerCallback;
    m_DebugUtilsMessenger = CreateDebugUtilsMessengerEXT(m_VulkanInstance, debugUtilsMessengerInfo);
}

VulkanContext::~VulkanContext()
{
    m_Swapchain->Destroy();
    vkDestroySurfaceKHR(m_VulkanInstance, m_Surface, nullptr);
    m_Device->Destroy();
    DestroyDebugUtilsMessengerEXT(m_VulkanInstance, m_DebugUtilsMessenger);
    if (m_VulkanInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_VulkanInstance, nullptr);
        m_VulkanInstance = VK_NULL_HANDLE;
    }
}

VulkanContext& VulkanContext::Init(SurfaceCreateFunc&& surfaceCreateFunc, uint32_t width, uint32_t height, bool vsync)
{
    m_Instance = new VulkanContext;
    m_Instance->InitDevice();
    m_Instance->InitSurface(std::move(surfaceCreateFunc));
    m_Instance->InitSwapchain(width, height, vsync);
    return *m_Instance;
}

void VulkanContext::Shutdown()
{
    delete m_Instance;
}

void VulkanContext::InitDevice()
{
    m_Device = CreateRef<VulkanDevice>();
}

void VulkanContext::InitSurface(SurfaceCreateFunc&& surfaceCreateFunc)
{
    m_Surface = surfaceCreateFunc(m_VulkanInstance);
    m_Device->SetupPresentQueue(m_Surface);
}

void VulkanContext::InitSwapchain(uint32_t width, uint32_t height, bool vsync)
{
    m_Swapchain = CreateRef<VulkanSwapchain>(m_Device.get(), width, height, vsync);
}

}
