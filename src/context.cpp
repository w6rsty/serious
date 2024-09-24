#include "serious/context.hpp"

namespace serious {

Context::Context() {

}

Context::~Context() {
    if (device) {
        device.waitIdle();

        destroySyncObjects();
        destroyCommandPool();
        destroyFramebuffers();
        destroyPipeline();
        destroySwapchain();
        device.destroy();
    }
    if (surface) {
        instance.destroySurfaceKHR(surface);
    }
    instance.destroy();
}

void Context::Prepare(const std::vector<const char*>& extensions, const WindowOptions& options) {
    createInstance(extensions, options);

    /// Create Surface from Window
    createSurface(options.create_surface_callback);

    /// Select GPU, create device, get queues
    selectPhysicalDevice();
    createDevice({VK_KHR_SWAPCHAIN_EXTENSION_NAME});

    /// Swapchain(Images, ImageViews)
    createSwapchain(options.extent);

    /// RenderPass, PipelineLayout, Pipeline
    createPipeline(options.extent);
    createFramebuffers();
    
    /// CommandPool
    createCommandPool();
    createSyncObjects();
}

void Context::Update() {
    drawFrame();
}

}
