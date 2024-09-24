#include "serious/context.hpp"

#include "serious/tool.hpp"

#include <array>
#include <limits>

namespace serious {

static constexpr uint64_t WAIT_TIME = std::numeric_limits<uint64_t>::max();

void Context::createCommandPool() {
    vk::CommandPoolCreateInfo create_info;
    create_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
               .setQueueFamilyIndex(queue_indices.graphics.value());
    command_pool = device.createCommandPool(create_info);
    
    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.setCommandPool(command_pool)
                 .setLevel(vk::CommandBufferLevel::ePrimary)
                 .setCommandBufferCount(1);
    command_buffer = device.allocateCommandBuffers(allocate_info)[0];
}

void Context::destroyCommandPool() {
    device.freeCommandBuffers(command_pool, command_buffer);
    device.destroyCommandPool(command_pool);
}

void Context::createSyncObjects() {
    vk::FenceCreateInfo fence_info;
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
    queue_submit_fence = device.createFence(fence_info);
    vk::SemaphoreCreateInfo semaphore_info;
    swapchain_acquire_semaphore = device.createSemaphore(semaphore_info);;
    swapchain_release_semaphore = device.createSemaphore(semaphore_info);;
}

void Context::destroySyncObjects() {
    device.destroyFence(queue_submit_fence);
    device.destroySemaphore(swapchain_acquire_semaphore);
    device.destroySemaphore(swapchain_release_semaphore);
}   

void Context::drawFrame() {
    if (device.waitForFences(1, &queue_submit_fence, true, WAIT_TIME)
            != vk::Result::eSuccess) {
        FatalError("failed to wait for queue submit fence");
    }
    if (device.resetFences(1, &queue_submit_fence) != vk::Result::eSuccess) {
        FatalError("failed to reset queue submit fence");
    }

    uint32_t image_index;
    if (device.acquireNextImageKHR(
                swapchain_data.swapchain,
                WAIT_TIME,
                swapchain_acquire_semaphore,
                queue_submit_fence,
                &image_index) != vk::Result::eSuccess) {
        FatalError("failed to acquire image from swapchain");
    }

    command_buffer.reset();

    /// Recored commands
    renderTriangle(image_index);

    /// Submit queue
    vk::PipelineStageFlags wait_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(command_buffer)
          .setWaitSemaphores(swapchain_acquire_semaphore)
          .setSignalSemaphores(swapchain_release_semaphore)
          .setWaitDstStageMask(wait_stages);

    graphics_queue.submit(submit_info);

    /// Present
    vk::PresentInfoKHR present_info;
    present_info.setImageIndices(image_index)
                .setSwapchains(swapchain_data.swapchain)
                .setWaitSemaphores(swapchain_release_semaphore);

    if (present_queue.presentKHR(present_info) != vk::Result::eSuccess) {
        FatalError("failed to present image to surface");
    }
}

void Context::renderTriangle(uint32_t image_index) {
    vk::Framebuffer framebuffer = swapchain_data.framebuffers[image_index];

    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    command_buffer.begin(begin_info); 

    vk::ClearValue clear_value;
    clear_value.color = vk::ClearColorValue(std::array<float, 4>({{0.01f, 0.01f, 0.033f, 1.0f}}));

    vk::RenderPassBeginInfo renderpass_begin;
    renderpass_begin.setRenderArea({{0, 0}, swapchain_data.extent})
                    .setFramebuffer(framebuffer)
                    .setRenderPass(render_pass)
                    .setClearValues(clear_value);
    command_buffer.beginRenderPass(renderpass_begin, vk::SubpassContents {}); 
    
    command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
    command_buffer.draw(3, 1, 0, 0);
    
    command_buffer.endRenderPass();

    command_buffer.end();
}

}
