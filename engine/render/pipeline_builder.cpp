#include "pipeline_builder.h"
#include "io/logging.h"
#include <vulkan/vulkan_core.h>

PipelineBuilder::PipelineBuilder() : shader_stages() {
}

VkPipeline PipelineBuilder::build(VkDevice device, VkRenderPass render_pass) {
  if (vertex_input_info.vertexAttributeDescriptionCount == 3) {
    io::info(
        "desc[0] format: {}",
        (uint32_t)vertex_input_info.pVertexAttributeDescriptions[0].format);
    io::info(
        "desc[0] format: {}",
        (uint32_t)vertex_input_info.pVertexAttributeDescriptions[1].format);
    io::info(
        "desc[0] format: {}",
        (uint32_t)vertex_input_info.pVertexAttributeDescriptions[2].format);
  }

  VkPipelineViewportStateCreateInfo viewport_state = {};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.pNext = nullptr;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &this->viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &this->scissor;

  VkPipelineColorBlendStateCreateInfo color_blending = {};
  color_blending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.pNext = nullptr;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &this->color_blend_attachment;

  VkGraphicsPipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.pNext = nullptr;
  pipeline_info.stageCount = this->shader_stages.size();
  pipeline_info.pStages = this->shader_stages.data();
  pipeline_info.pVertexInputState = &this->vertex_input_info;
  pipeline_info.pInputAssemblyState = &this->input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &this->rasterizer;
  pipeline_info.pMultisampleState = &this->multisampling;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.layout = this->pipeline_layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  VkPipeline pipeline;
  if (vkCreateGraphicsPipelines(
          device,
          VK_NULL_HANDLE,
          1,
          &pipeline_info,
          nullptr,
          &pipeline)) {
    io::error("Failed to create pipeline.");
    return VK_NULL_HANDLE;
  } else {
    return pipeline;
  }
}

PipelineBuilder &PipelineBuilder::clear_shader_stages() {
  this->shader_stages.clear();

  return *this;
}

PipelineBuilder &PipelineBuilder::add_shader_stage(
    VkPipelineShaderStageCreateInfo shader_stage) {
  this->shader_stages.push_back(shader_stage);

  return *this;
}

PipelineBuilder &PipelineBuilder::with_vertex_input(
    VkPipelineVertexInputStateCreateInfo vertex_input_info) {
  this->vertex_input_info = vertex_input_info;

  return *this;
}

PipelineBuilder &PipelineBuilder::with_input_assembly(
    VkPipelineInputAssemblyStateCreateInfo input_assembly) {
  this->input_assembly = input_assembly;

  return *this;
}

PipelineBuilder &PipelineBuilder::with_viewport(VkViewport viewport) {
  this->viewport = viewport;

  return *this;
}

PipelineBuilder &PipelineBuilder::with_scissor(VkRect2D scissor) {
  this->scissor = scissor;

  return *this;
}

PipelineBuilder &PipelineBuilder::with_rasterizer(
    VkPipelineRasterizationStateCreateInfo rasterizer) {
  this->rasterizer = rasterizer;

  return *this;
}

PipelineBuilder &PipelineBuilder::with_color_blend_attachment(
    VkPipelineColorBlendAttachmentState color_blend_attachment) {
  this->color_blend_attachment = color_blend_attachment;

  return *this;
}

PipelineBuilder &PipelineBuilder::with_multisample(
    VkPipelineMultisampleStateCreateInfo multisampling) {
  this->multisampling = multisampling;

  return *this;
}

PipelineBuilder &
PipelineBuilder::with_pipeline_layout(VkPipelineLayout pipeline_layout) {
  this->pipeline_layout = pipeline_layout;

  return *this;
}
