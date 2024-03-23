#pragma once

#include "vk_types.h"

#include <vector>
#include <vulkan/vulkan_core.h>

class PipelineBuilder {
public:
  PipelineBuilder();

  VkPipeline build(VkDevice device, VkRenderPass render_pass);

  PipelineBuilder &
  add_shader_stage(VkPipelineShaderStageCreateInfo shader_stage);

  PipelineBuilder &
  with_vertex_input(VkPipelineVertexInputStateCreateInfo vertex_input_info);

  PipelineBuilder &
  with_input_assembly(VkPipelineInputAssemblyStateCreateInfo input_assembly);

  PipelineBuilder &with_viewport(VkViewport viewport);

  PipelineBuilder &with_scissor(VkRect2D scissor);

  PipelineBuilder &
  with_rasterizer(VkPipelineRasterizationStateCreateInfo rasterizer);

  PipelineBuilder &with_color_blend_attachment(
      VkPipelineColorBlendAttachmentState color_blend_attachment);

  PipelineBuilder &
  with_multisample(VkPipelineMultisampleStateCreateInfo multisampling);

  PipelineBuilder &with_pipeline_layout(VkPipelineLayout pipeline_layout);

private:
  std::vector<VkPipelineShaderStageCreateInfo> shader_stages;
  VkPipelineVertexInputStateCreateInfo vertex_input_info;
  VkPipelineInputAssemblyStateCreateInfo input_assembly;
  VkViewport viewport;
  VkRect2D scissor;
  VkPipelineRasterizationStateCreateInfo rasterizer;
  VkPipelineColorBlendAttachmentState color_blend_attachment;
  VkPipelineMultisampleStateCreateInfo multisampling;
  VkPipelineLayout pipeline_layout;
};
