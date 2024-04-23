#include "vk_init.h"

#include "io/logging.h"
#include "render/tri_mesh.h"
#include "vk_types.h"
#include <vector>
#include <vulkan/vulkan_core.h>

VkAttachmentDescription VkInit::color_attachment(VkFormat format) {
  VkAttachmentDescription color_attachment = {};
  color_attachment.format = format;
  color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  return color_attachment;
}

VkAttachmentReference VkInit::color_attachment_ref() {
  VkAttachmentReference color_attachment_ref = {};
  color_attachment_ref.attachment = 0;
  color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  return color_attachment_ref;
}

VkAttachmentDescription VkInit::depth_attachment(VkFormat format) {
  VkAttachmentDescription depth_attachment = {};
  depth_attachment.flags = 0;
  depth_attachment.format = format;
  depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  return depth_attachment;
}

VkAttachmentReference VkInit::depth_attachment_ref() {
  VkAttachmentReference depth_attachment_ref = {};
  depth_attachment_ref.attachment = 1;
  depth_attachment_ref.layout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  return depth_attachment_ref;
}

VkSubpassDescription VkInit::subpass_description(
    VkPipelineBindPoint bind_point,
    VkAttachmentReference *color_attachment_ref,
    VkAttachmentReference *depth_attachment_ref) {
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = bind_point;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = color_attachment_ref;
  subpass.pDepthStencilAttachment = depth_attachment_ref;

  return subpass;
}

VkRenderPassCreateInfo VkInit::render_pass_create_info(
    std::vector<VkAttachmentDescription> *attachments,
    std::vector<VkSubpassDependency> *dependencies,
    VkSubpassDescription *subpass) {
  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = attachments->size();
  info.pAttachments = attachments->data();
  info.subpassCount = 1;
  info.pSubpasses = subpass;
  info.dependencyCount = dependencies->size();
  info.pDependencies = dependencies->data();

  return info;
}

VkFramebufferCreateInfo VkInit::frame_buffer_create_info(
    VkRenderPass render_pass,
    Dimensions dimensions) {
  VkFramebufferCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  info.pNext = nullptr;
  info.renderPass = render_pass;
  info.attachmentCount = 1;
  info.width = dimensions.width;
  info.height = dimensions.height;
  info.layers = 1;

  return info;
}

VkCommandPoolCreateInfo VkInit::command_pool_create_info(
    uint32_t queue_family_index,
    VkCommandPoolCreateFlags flags) {
  VkCommandPoolCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  info.pNext = nullptr;
  info.queueFamilyIndex = queue_family_index;
  info.flags = flags;

  return info;
}

VkCommandBufferAllocateInfo VkInit::command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count,
    VkCommandBufferLevel level) {
  VkCommandBufferAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  info.pNext = nullptr;
  info.commandPool = pool;
  info.commandBufferCount = count;
  info.level = level;

  return info;
}

VkCommandBufferBeginInfo
VkInit::command_buffer_begin_info(VkCommandBufferUsageFlags flags) {
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.pNext = nullptr;
  info.pInheritanceInfo = nullptr;
  info.flags = flags;

  return info;
}

VkRenderPassBeginInfo VkInit::render_pass_begin_info(
    VkRenderPass render_pass,
    VkFramebuffer frame_buffer,
    std::vector<VkClearValue> *clear_values,
    Dimensions dimensions) {
  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.pNext = nullptr;
  info.renderPass = render_pass;
  info.renderArea.offset.x = 0;
  info.renderArea.offset.y = 0;
  info.renderArea.extent = {dimensions.width, dimensions.height};
  info.framebuffer = frame_buffer;
  info.clearValueCount = clear_values->size();
  info.pClearValues = clear_values->data();

  return info;
}

VkSubmitInfo VkInit::submit_info(
    VkSemaphore *wait_semaphore,
    VkSemaphore *signal_semaphore,
    VkCommandBuffer *cmd,
    VkPipelineStageFlags *flags) {
  VkSubmitInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  info.pNext = nullptr;
  info.pWaitDstStageMask = flags;

  if (wait_semaphore != nullptr) {
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = wait_semaphore;
  } else {
    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = nullptr;
  }

  if (signal_semaphore != nullptr) {
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = signal_semaphore;
  } else {
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores = nullptr;
  }
  info.commandBufferCount = 1;
  info.pCommandBuffers = cmd;

  return info;
}

VkPresentInfoKHR VkInit::present_info(
    VkSwapchainKHR *swapchain,
    VkSemaphore *wait_semaphore,
    uint32_t *image_index) {
  VkPresentInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  info.pNext = nullptr;
  info.swapchainCount = 1;
  info.pSwapchains = swapchain;
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = wait_semaphore;
  info.pImageIndices = image_index;

  return info;
}

VkSemaphoreCreateInfo VkInit::semaphore_create_info() {
  VkSemaphoreCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = 0;

  return info;
}

VkFenceCreateInfo VkInit::fence_create_info(bool create_signaled) {
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.pNext = nullptr;
  if (create_signaled) {
    info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  }

  return info;
}

VkImageCreateInfo VkInit::image_create_info(
    VkFormat format,
    VkImageUsageFlags flags,
    VkExtent3D extent) {
  VkImageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  info.pNext = nullptr;
  info.imageType = VK_IMAGE_TYPE_2D;
  info.format = format;
  info.extent = extent;
  info.mipLevels = 1;
  info.arrayLayers = 1;
  info.samples = VK_SAMPLE_COUNT_1_BIT;
  info.tiling = VK_IMAGE_TILING_OPTIMAL;
  info.usage = flags;

  return info;
}

VkImageViewCreateInfo VkInit::imageview_create_info(
    VkFormat format,
    VkImage image,
    VkImageAspectFlags flags) {
  VkImageViewCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  info.pNext = nullptr;
  info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  info.image = image;
  info.format = format;
  info.subresourceRange.baseMipLevel = 0;
  info.subresourceRange.levelCount = 1;
  info.subresourceRange.baseArrayLayer = 0;
  info.subresourceRange.layerCount = 1;
  info.subresourceRange.aspectMask = flags;

  return info;
}

VkPipelineShaderStageCreateInfo VkInit::pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage,
    VkShaderModule shader_module) {
  VkPipelineShaderStageCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  info.pNext = nullptr;
  info.stage = stage;
  info.module = shader_module;
  info.pName = "main";

  return info;
}

VkPipelineVertexInputStateCreateInfo
VkInit::vertex_input_state_create_info(VertexInputDescription *vertex_input) {
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.pNext = nullptr;

  if (vertex_input != nullptr) {
    info.vertexBindingDescriptionCount = vertex_input->bindings.size();
    info.pVertexBindingDescriptions = vertex_input->bindings.data();
    info.vertexAttributeDescriptionCount = vertex_input->attributes.size();
    info.pVertexAttributeDescriptions = vertex_input->attributes.data();
  } else {
    info.vertexBindingDescriptionCount = 0;
    info.vertexAttributeDescriptionCount = 0;
  }

  return info;
}

VkPipelineInputAssemblyStateCreateInfo
VkInit::input_assembly_state_create_info(VkPrimitiveTopology topology) {
  VkPipelineInputAssemblyStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.topology = topology;
  info.primitiveRestartEnable = VK_FALSE;

  return info;
}

VkPipelineRasterizationStateCreateInfo
VkInit::rasterization_state_create_info(VkPolygonMode polygon_mode) {
  VkPipelineRasterizationStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.depthClampEnable = VK_FALSE;
  info.rasterizerDiscardEnable = VK_FALSE;
  info.polygonMode = polygon_mode;
  info.lineWidth = 1.0f;
  info.cullMode = VK_CULL_MODE_BACK_BIT;
  info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  info.depthBiasEnable = VK_FALSE;
  info.depthBiasConstantFactor = 0.0f;
  info.depthBiasClamp = 0.0f;
  info.depthBiasSlopeFactor = 0.0f;

  return info;
}

VkPipelineMultisampleStateCreateInfo VkInit::multisample_state_create_info() {
  VkPipelineMultisampleStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.sampleShadingEnable = VK_FALSE;
  info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  info.minSampleShading = 1.0f;
  info.pSampleMask = nullptr;
  info.alphaToCoverageEnable = VK_FALSE;
  info.alphaToOneEnable = VK_FALSE;

  return info;
}

VkPipelineColorBlendAttachmentState VkInit::color_blend_attachment_state() {
  VkPipelineColorBlendAttachmentState color_blend_attachment = {};
  color_blend_attachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  return color_blend_attachment;
}

VkPipelineLayoutCreateInfo VkInit::pipeline_layout_create_info(
    std::vector<VkPushConstantRange> *push_constants,
    std::vector<VkDescriptorSetLayout> *descriptor_layouts) {
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = 0;
  info.setLayoutCount = 0;
  info.pSetLayouts = nullptr;

  if (push_constants != nullptr) {
    info.pushConstantRangeCount = push_constants->size();
    info.pPushConstantRanges = push_constants->data();
  } else {
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
  }

  if (descriptor_layouts != nullptr) {
    info.setLayoutCount = descriptor_layouts->size();
    info.pSetLayouts = descriptor_layouts->data();
  } else {
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
  }

  return info;
}

VkPipelineDepthStencilStateCreateInfo VkInit::depth_stencil_create_info(
    bool depth_test,
    bool depth_write,
    VkCompareOp op) {
  VkPipelineDepthStencilStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.depthTestEnable = depth_test ? VK_TRUE : VK_FALSE;
  info.depthWriteEnable = depth_write ? VK_TRUE : VK_FALSE;
  info.depthCompareOp = depth_test ? op : VK_COMPARE_OP_ALWAYS;
  info.depthBoundsTestEnable = VK_FALSE;
  info.minDepthBounds = 0.0f;
  info.maxDepthBounds = 1.0f;
  info.stencilTestEnable = VK_FALSE;

  return info;
}

AllocatedBuffer VkInit::buffer(
    VmaAllocator allocator,
    uint32_t size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memory_usage) {
  AllocatedBuffer buffer = {};

  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext = nullptr;
  buffer_info.size = size;
  buffer_info.usage = usage;

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = memory_usage;

  VK_ASSERT(vmaCreateBuffer(
      allocator,
      &buffer_info,
      &alloc_info,
      &buffer.buffer,
      &buffer.allocation,
      nullptr));

  buffer.range = size;

  return buffer;
}

VkDescriptorSetLayoutBinding VkInit::descriptor_set_layout_binding(
    VkDescriptorType type,
    VkShaderStageFlags stage,
    uint32_t binding,
    uint32_t count) {
  VkDescriptorSetLayoutBinding set_binding = {};
  set_binding.binding = binding;
  set_binding.descriptorCount = 1;
  set_binding.descriptorType = type;
  set_binding.stageFlags = stage;
  set_binding.descriptorCount = count;

  return set_binding;
}

VkWriteDescriptorSet VkInit::write_descriptor_set(
    VkDescriptorType type,
    VkDescriptorSet dest,
    VkDescriptorBufferInfo *info,
    uint32_t binding) {
  VkWriteDescriptorSet write_set = {};
  write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_set.pNext = nullptr;
  write_set.dstBinding = binding;
  write_set.dstSet = dest;
  write_set.descriptorCount = 1;
  write_set.descriptorType = type;
  write_set.pBufferInfo = info;

  return write_set;
}

VkSamplerCreateInfo VkInit::sampler_create_info(
    VkFilter filters,
    VkSamplerAddressMode address_mode) {
  VkSamplerCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  info.pNext = nullptr;
  info.magFilter = filters;
  info.minFilter = filters;
  info.addressModeU = address_mode;
  info.addressModeV = address_mode;
  info.addressModeW = address_mode;

  return info;
}

VkWriteDescriptorSet VkInit::write_descriptor_image(
    VkDescriptorType type,
    VkDescriptorSet dest,
    VkDescriptorImageInfo *info,
    uint32_t binding,
    uint32_t index) {
  VkWriteDescriptorSet write_set = {};
  write_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write_set.pNext = nullptr;
  write_set.dstBinding = binding;
  write_set.dstSet = dest;
  write_set.dstArrayElement = index;
  write_set.descriptorCount = 1;
  write_set.descriptorType = type;
  write_set.pImageInfo = info;

  return write_set;
}

VkDescriptorSetAllocateInfo VkInit::descriptor_set_allocate_info(
    VkDescriptorPool &pool,
    VkDescriptorSetLayout &layout) {
  VkDescriptorSetAllocateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  info.pNext = nullptr;
  info.descriptorPool = pool;
  info.descriptorSetCount = 1;
  info.pSetLayouts = &layout;

  return info;
}
