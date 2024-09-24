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
    
    frame_datas.resize(MAX_FRAMES_IN_FLIGHT);

    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.setCommandPool(command_pool)
                 .setLevel(vk::CommandBufferLevel::ePrimary)
                 .setCommandBufferCount(MAX_FRAMES_IN_FLIGHT);
    auto command_buffers = device.allocateCommandBuffers(allocate_info);
    for (int i = 0; i < frame_datas.size(); ++i) {
        frame_datas[i].cmd = command_buffers[i];
    }
}

void Context::createSyncObjects() {
    vk::FenceCreateInfo fence_info;
    fence_info.setFlags(vk::FenceCreateFlagBits::eSignaled);
    vk::SemaphoreCreateInfo semaphore_info;
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        frame_datas[i].queue_submit_fence = device.createFence(fence_info);
        frame_datas[i].swapchain_acquire_semaphore = device.createSemaphore(semaphore_info);
        frame_datas[i].swapchain_release_semaphore = device.createSemaphore(semaphore_info);
    }
}

void Context::drawFrame() {
    auto& frame_data = frame_datas[current_frame];
    if (device.waitForFences(1, &frame_data.queue_submit_fence, true, WAIT_TIME)
            != vk::Result::eSuccess) {
        FatalError("failed to wait for queue submit fence");
    }
    if (device.resetFences(1, &frame_data.queue_submit_fence) != vk::Result::eSuccess) {
        FatalError("failed to reset queue submit fence");
    }

    uint32_t image_index;
    if (device.acquireNextImageKHR(
                swapchain_data.swapchain,
                WAIT_TIME,
                frame_data.swapchain_acquire_semaphore,
                frame_data.queue_submit_fence,
                &image_index) != vk::Result::eSuccess) {
        FatalError("failed to acquire image from swapchain");
    }

    frame_data.cmd.reset();

    /// Recored commands
    renderTriangle(image_index);

    /// Submit command to graphics queue
    vk::PipelineStageFlags wait_stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::SubmitInfo submit_info;
    submit_info.setCommandBuffers(frame_data.cmd)
          .setWaitSemaphores(frame_data.swapchain_acquire_semaphore)
          .setSignalSemaphores(frame_data.swapchain_release_semaphore)
          .setWaitDstStageMask(wait_stages);
    graphics_queue.submit(submit_info);

    /// Present to surface
    vk::PresentInfoKHR present_info;
    present_info.setImageIndices(image_index)
                .setSwapchains(swapchain_data.swapchain)
                .setWaitSemaphores(frame_data.swapchain_release_semaphore);
    if (present_queue.presentKHR(present_info) != vk::Result::eSuccess) {
        FatalError("failed to present image to surface");
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Context::renderTriangle(uint32_t image_index) {
    auto& frame_data = frame_datas[current_frame];

    vk::Framebuffer framebuffer = swapchain_data.framebuffers[image_index];

    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    frame_data.cmd.begin(begin_info); 
        vk::ClearValue clear_value;
        clear_value.color = vk::ClearColorValue(std::array<float, 4>({{0.1f, 0.1f, 0.1f, 1.0f}}));

        vk::RenderPassBeginInfo renderpass_begin;
        renderpass_begin.setRenderArea({{0, 0}, swapchain_data.extent})
                        .setFramebuffer(framebuffer)
                        .setRenderPass(render_pass)
                        .setClearValues(clear_value);
        frame_data.cmd.beginRenderPass(renderpass_begin, vk::SubpassContents {}); 
            frame_data.cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
            frame_data.cmd.draw(6, 2, 0, 0);
        frame_data.cmd.endRenderPass();
    frame_data.cmd.end();
}

void Context::destroySyncObjects() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        device.destroyFence(frame_datas[i].queue_submit_fence);
        device.destroySemaphore(frame_datas[i].swapchain_acquire_semaphore);
        device.destroySemaphore(frame_datas[i].swapchain_release_semaphore);
    }
}   

void Context::destroyCommandPool() {
    for (int i = 0; i < frame_datas.size(); ++i) {
        device.freeCommandBuffers(command_pool, frame_datas[i].cmd);
    }
    device.destroyCommandPool(command_pool);
}

}
