#include "serious/context.hpp"
#include "serious/tool.hpp"

namespace serious {

void Context::createRenderPass() {
    vk::AttachmentDescription color_attachment;
    color_attachment.setFormat(swapchain_data.surface_format.format)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setLoadOp(vk::AttachmentLoadOp::eClear)
                    .setStoreOp(vk::AttachmentStoreOp::eStore)
                    .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                    .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference color_attachment_ref;
    color_attachment_ref.setAttachment(0)
                        .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
           .setColorAttachments(color_attachment_ref);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
              .setDstSubpass(0)
              .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
              .setSrcAccessMask(vk::AccessFlagBits::eNone)
              .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
              .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo create_info;
    create_info.setAttachments(color_attachment)
               .setSubpasses(subpass)
               .setDependencies(dependency);
    render_pass = device.createRenderPass(create_info);   
}

void Context::createPipelineLayout() {
    vk::PipelineLayoutCreateInfo create_info;
    pipeline_layout = device.createPipelineLayout(create_info);
}

void Context::createPipeline(vk::Extent2D window_extent) {
    auto vert_shader_source = ReadFile("shaders/vert.spv");
    vk::ShaderModuleCreateInfo shader_module_info;
    shader_module_info.pCode = (uint32_t*)vert_shader_source.data();
    shader_module_info.codeSize = vert_shader_source.size();
    vert_shader_module = device.createShaderModule(shader_module_info); 

    auto frag_shader_source = ReadFile("shaders/frag.spv");
    shader_module_info.pCode = (uint32_t*)frag_shader_source.data();
    shader_module_info.codeSize = frag_shader_source.size();
    frag_shader_module = device.createShaderModule(shader_module_info);

    vk::PipelineShaderStageCreateInfo vert_shader_stage;
    vert_shader_stage.setStage(vk::ShaderStageFlagBits::eVertex)
                     .setModule(vert_shader_module)
                     .setPName("main");

    vk::PipelineShaderStageCreateInfo frag_shader_stage;
    frag_shader_stage.setStage(vk::ShaderStageFlagBits::eFragment)
                     .setModule(frag_shader_module)
                     .setPName("main");

    std::array shader_stages = {vert_shader_stage, frag_shader_stage};

    vk::PipelineVertexInputStateCreateInfo vertex_input;

    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly.setPrimitiveRestartEnable(false)
                  .setTopology(vk::PrimitiveTopology::eTriangleList);
    
    vk::Viewport viewport(0.0f, 0.0f, window_extent.width, window_extent.height, 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, window_extent);
    vk::PipelineViewportStateCreateInfo viewport_state;
    viewport_state.setViewports(viewport)
                  .setScissors(scissor);

    vk::PipelineRasterizationStateCreateInfo rasterization;
    rasterization.setDepthClampEnable(false)
                 .setDepthBiasEnable(false)
                 .setRasterizerDiscardEnable(false)
                 .setPolygonMode(vk::PolygonMode::eFill)
                 .setLineWidth(1.0f)
                 .setCullMode(vk::CullModeFlagBits::eBack)
                 .setFrontFace(vk::FrontFace::eClockwise);

    vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.setSampleShadingEnable(false)
               .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState color_blend_attachment;
    color_blend_attachment.setColorWriteMask(vk::ColorComponentFlagBits::eR |
                                             vk::ColorComponentFlagBits::eG |
                                             vk::ColorComponentFlagBits::eB |
                                             vk::ColorComponentFlagBits::eA)
                          .setBlendEnable(false);
    vk::PipelineColorBlendStateCreateInfo color_blend_state;
    color_blend_state.setLogicOpEnable(false)
                     .setAttachments(color_blend_attachment);

    createRenderPass();
    createPipelineLayout();

    vk::GraphicsPipelineCreateInfo create_info;
    create_info.setStages(shader_stages)
               .setPVertexInputState(&vertex_input)
               .setPInputAssemblyState(&input_assembly)
               .setPViewportState(&viewport_state)
               .setPRasterizationState(&rasterization)
               .setPMultisampleState(&multisample)
               .setPColorBlendState(&color_blend_state)
               .setRenderPass(render_pass)
               .setLayout(pipeline_layout);
    auto result = device.createGraphicsPipeline(nullptr, create_info);
    if (result.result != vk::Result::eSuccess) {
        FatalError("failed to create graphics pipeline");
    }
    pipeline = result.value;
}

void Context::destroyPipeline() {
    if (render_pass) {
        device.destroyPipeline(pipeline);
        device.destroyPipelineLayout(pipeline_layout);
        device.destroyRenderPass(render_pass);

        device.destroyShaderModule(frag_shader_module);
        device.destroyShaderModule(vert_shader_module);
    }
}

}
