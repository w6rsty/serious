#pragma once

#include <vulkan/vulkan.hpp>

#include <optional>
#include <functional>
#include <cstdint>

namespace serious {

using CreateSurfaceCallback = std::function<vk::SurfaceKHR(vk::Instance)>;

struct WindowOptions {
    vk::Extent2D extent;
    CreateSurfaceCallback create_surface_callback;
};


class Context final {
public:
    Context();
    ~Context();
    Context(const Context&) = delete;
    Context& operator = (const Context&) = delete;
    Context(Context&&) = default;
    Context& operator = (Context&&) = default;

    void Prepare(const std::vector<const char*>& surface_extensions, const WindowOptions& options);
    void Update();
private:
    void createInstance(const std::vector<const char*>& surface_extensions, const WindowOptions& options);
    void createSurface(CreateSurfaceCallback callback);
    void selectPhysicalDevice();
    void createDevice(const std::vector<const char*>& device_extensions);
    void createSwapchain(vk::Extent2D window_extent);
    void destroySwapchain();
    void createRenderPass();
    void createPipelineLayout();
    void createPipeline(vk::Extent2D window_extent);
    void destroyPipeline();
    void createFramebuffers();
    void destroyFramebuffers();
    void createCommandPool();
    void destroyCommandPool();
    void createSyncObjects();
    void destroySyncObjects();
    void drawFrame();
    void renderTriangle(uint32_t image_index);
private:
    struct SwapchainData {
        vk::SwapchainKHR swapchain;
        vk::SurfaceFormatKHR surface_format;
        uint32_t image_count;
        vk::Extent2D extent;
        vk::PresentModeKHR present_mode;
        std::vector<vk::Image> images;
        std::vector<vk::ImageView> image_view;
        std::vector<vk::Framebuffer> framebuffers;
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;

        inline bool completed() const { return graphics.has_value() && present.has_value(); } 
        inline bool shared() const { return graphics.value() == present.value(); }
    };

    vk::Instance instance;
    vk::SurfaceKHR surface;
    QueueFamilyIndices queue_indices;
    vk::PhysicalDevice gpu;
    vk::Device device;
    vk::Queue graphics_queue;
    vk::Queue present_queue;
    SwapchainData swapchain_data; 

    vk::ShaderModule vert_shader_module;
    vk::ShaderModule frag_shader_module;
    vk::RenderPass render_pass;
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;

    vk::CommandPool command_pool;
    vk::CommandBuffer command_buffer;

    vk::Fence queue_submit_fence;
    vk::Semaphore swapchain_acquire_semaphore;
    vk::Semaphore swapchain_release_semaphore;
};

}
