#include "serious/context.hpp"
#include "serious/tool.hpp"

#include <vector>

namespace serious {

/// From https://github.com/KhronosGroup/Vulkan-Samples/blob
static inline bool validateExtensions(
        const std::vector<const char*>& required,
        const std::vector<vk::ExtensionProperties>& available) {
	return std::find_if(required.begin(), required.end(),
        [&available](auto extension) {
            return std::find_if(available.begin(), available.end(),
                [&extension](auto const &ep) {
                    return strcmp(ep.extensionName, extension) == 0;
                }) == available.end();
        }) == required.end();
}

void Context::createSurface(CreateSurfaceCallback callback) {
    surface = callback(instance);

    if (!surface) {
        FatalError("Failed to create surface");
    }
}

void Context::selectPhysicalDevice() {
    auto gpus = instance.enumeratePhysicalDevices();

    bool found_suitable_gpu = false;
    for (int i = 0; i < gpus.size() && !found_suitable_gpu; ++i) {
        gpu = gpus[i];
        auto queue_famliy_properties = gpu.getQueueFamilyProperties();

        for (int j = 0; j < queue_famliy_properties.size(); ++j) {
            auto& property = queue_famliy_properties[j]; 
            if (property.queueFlags & vk::QueueFlagBits::eGraphics) {
                queue_indices.graphics = j;
            }
            if (gpu.getSurfaceSupportKHR(j, surface)) {
                queue_indices.present = j;
            }
            if (queue_indices.completed()) {
                found_suitable_gpu = true;
                break;
            }
        }    
    }

    if (!found_suitable_gpu) {
        FatalError("No supported GPU found");
    }
    std::string name = gpu.getProperties().deviceName;
    Info("using device: {}", name);
}

void Context::createDevice(const std::vector<const char*>& device_extensions) {
   auto available_device_extentions = gpu.enumerateDeviceExtensionProperties();
    if (!validateExtensions(device_extensions, available_device_extentions)) {
        FatalError("required device extension not supported by current device");
    }

    float priority = 1.0f;
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    if (queue_indices.shared()) {
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.setQueuePriorities(priority)
                         .setQueueCount(1)
                         .setQueueFamilyIndex(queue_indices.graphics.value());
        queue_create_infos.push_back(queue_create_info);
    } else {
        vk::DeviceQueueCreateInfo queue_create_info;
        queue_create_info.setQueuePriorities(priority)
                         .setQueueCount(1)
                         .setQueueFamilyIndex(queue_indices.graphics.value());
        queue_create_infos.push_back(queue_create_info);
        queue_create_info.setQueuePriorities(priority)
                         .setQueueCount(1)
                         .setQueueFamilyIndex(queue_indices.present.value());
        queue_create_infos.push_back(queue_create_info);
    }

    vk::DeviceCreateInfo create_info;
    create_info.setQueueCreateInfos(queue_create_infos);
    create_info.setPEnabledExtensionNames(device_extensions);
    device = gpu.createDevice(create_info);

    /// Get graphics and present queue
    graphics_queue = device.getQueue(queue_indices.graphics.value(), 0);
    present_queue = device.getQueue(queue_indices.present.value(), 0);
}

}
