#include "serious/context.hpp"

namespace serious {

void Context::createSwapchain(vk::Extent2D window_extent) {
    /// Surface formats(image format and colorspace)
    auto supported_formats = gpu.getSurfaceFormatsKHR(surface);
    swapchain_data.surface_format = supported_formats[0];
    for (const auto& surface_format : supported_formats) {
        if (surface_format.format == vk::Format::eR8G8B8Srgb &&
            surface_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            swapchain_data.surface_format = surface_format;
            break;
        }
    }

    /// Surface capabilities
    auto capabilities = gpu.getSurfaceCapabilitiesKHR(surface);
    swapchain_data.image_count = std::clamp<uint32_t>(2, capabilities.minImageCount,
            capabilities.maxImageCount);
    swapchain_data.extent.width = std::clamp<uint32_t>(window_extent.width,
            capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    swapchain_data.extent.height = std::clamp<uint32_t>(window_extent.height,
            capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    /// Surface present mode
    auto supported_present_modes = gpu.getSurfacePresentModesKHR(surface);
    swapchain_data.present_mode = vk::PresentModeKHR::eFifo; /// This mode is guaranteed to be available
    for (const auto& present_mode : supported_present_modes) {
        if (present_mode == vk::PresentModeKHR::eMailbox) {
            swapchain_data.present_mode = present_mode;
            break;
        }
    }
    
    /// Queue family indices
    std::vector<uint32_t> indices;
    if (queue_indices.shared()) {
        indices.push_back(queue_indices.graphics.value());
    } else {
        indices.push_back(queue_indices.graphics.value());
        indices.push_back(queue_indices.present.value());
    }

    vk::SwapchainCreateInfoKHR create_info;
    create_info.setClipped(true)
               .setImageArrayLayers(1)
               .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
               .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
               .setSurface(surface)
               .setImageFormat(swapchain_data.surface_format.format)
               .setImageColorSpace(swapchain_data.surface_format.colorSpace)
               .setMinImageCount(swapchain_data.image_count)
               .setImageExtent(swapchain_data.extent)
               .setPresentMode(swapchain_data.present_mode)
               .setQueueFamilyIndices(indices)
               .setImageSharingMode(queue_indices.shared() ? vk::SharingMode::eExclusive : 
                                                             vk::SharingMode::eConcurrent);
    swapchain_data.swapchain = device.createSwapchainKHR(create_info);

    /// Get images
    swapchain_data.images = device.getSwapchainImagesKHR(swapchain_data.swapchain);
    
    /// Create ImageViews
    swapchain_data.image_view.resize(swapchain_data.images.size());
    for (int i = 0; i < swapchain_data.images.size(); ++i) {
        vk::ComponentMapping mapping; /// Use default mapping
        vk::ImageSubresourceRange range;
        range.setAspectMask(vk::ImageAspectFlagBits::eColor)
             .setLayerCount(1)
             .setLevelCount(1)
             .setBaseMipLevel(0)
             .setBaseArrayLayer(0);

        vk::ImageViewCreateInfo image_view_create_info;
        image_view_create_info.setImage(swapchain_data.images[i])
                              .setFormat(swapchain_data.surface_format.format)
                              .setViewType(vk::ImageViewType::e2D)
                              .setComponents(mapping)
                              .setSubresourceRange(range);

        swapchain_data.image_view[i] = device.createImageView(image_view_create_info);
    }
}

void Context::createFramebuffers() {
    swapchain_data.framebuffers.resize(swapchain_data.image_view.size());

    for (int i = 0; i < swapchain_data.framebuffers.size(); ++i) {
        vk::FramebufferCreateInfo create_info;
        create_info.setAttachments(swapchain_data.image_view[i])
                   .setRenderPass(render_pass)
                   .setWidth(swapchain_data.extent.width)
                   .setHeight(swapchain_data.extent.height)
                   .setLayers(1);
        
        swapchain_data.framebuffers[i] = device.createFramebuffer(create_info);
    }
}

void Context::destroyFramebuffers() {
    for (auto& framebuffer : swapchain_data.framebuffers) {
        device.destroyFramebuffer(framebuffer);
    }
}

void Context::destroySwapchain() {

    for (auto& image_view : swapchain_data.image_view) {
        device.destroyImageView(image_view);
    }
    if (swapchain_data.swapchain) {
        device.destroySwapchainKHR(swapchain_data.swapchain);
    }
}

}
