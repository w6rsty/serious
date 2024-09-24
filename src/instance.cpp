#include "serious/context.hpp"
#include "serious/tool.hpp"

#include <vector>

namespace serious {

/// From https://github.com/KhronosGroup/Vulkan-Samples
static inline bool validateLayer(const std::vector<const char*>& required, const std::vector<vk::LayerProperties>& available) {
	auto requiredButNotFoundIt = std::find_if(required.begin(), required.end(),
        [&available](auto layer) {
            return std::find_if(available.begin(), available.end(),
                [&layer](auto const &lp) {
                    return strcmp(lp.layerName, layer) == 0;
                }) == available.end();
        });
	if (requiredButNotFoundIt != required.end())
	{
		FatalError("Validation Layer {} not found", *requiredButNotFoundIt);
	}
	return (requiredButNotFoundIt == required.end());
}

/// From https://github.com/KhronosGroup/Vulkan-Samples
static inline std::vector<const char*> getOptimalValidationLayer(const std::vector<vk::LayerProperties> &supported_instance_layers) {
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
		if (validateLayer(layer, supported_instance_layers))
		{
			return layer;
		}

		Warn("No supported validation layers");
	}
	return {};
}

void Context::createInstance(const std::vector<const char*>& surface_extensions, const WindowOptions& options) {
    /// Setup instance extensions
    auto avaliable_instance_extensions = vk::enumerateInstanceExtensionProperties();
    std::vector<const char*> enabled_instance_extensions;
    /// add surface extensions
    for (const auto& extension : surface_extensions) {
        enabled_instance_extensions.push_back(extension);
    }
    /// Check required extensions available
    for (const auto& extension : enabled_instance_extensions) {
        if (std::find_if(
            avaliable_instance_extensions.begin(),
            avaliable_instance_extensions.end(),
            [&avaliable_instance_extensions, &extension](const vk::ExtensionProperties& avaliable_extension) {
                return std::strcmp(avaliable_extension.extensionName, extension) == 0;
            }) == avaliable_instance_extensions.end()
        ) {
            FatalError("required extension {} not available", extension);
        }
    }


    /// Setup instance layers
    std::vector<const char*> enabled_instance_layer;
#ifdef ENABLE_VALIDATION
    auto validationLayers = getOptimalValidationLayer(vk::enumerateInstanceLayerProperties());
    enabled_instance_layer.insert(enabled_instance_layer.end(), validationLayers.begin(), validationLayers.end());
#endif


    /// Application info
    vk::ApplicationInfo app_info;
    app_info.setPApplicationName("serious")
            .setPEngineName("")
            .setApiVersion(VK_API_VERSION_1_3);
    
    /// Instance info
    vk::InstanceCreateInfo instance_info;
    instance_info.setPApplicationInfo(&app_info)
                 .setPEnabledExtensionNames(enabled_instance_extensions)
                 .setPEnabledLayerNames(enabled_instance_layer);
    instance = vk::createInstance(instance_info);
}

}
