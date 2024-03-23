#include "vk_init.h"

#include "vk_types.h"
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

VkSubpassDescription VkInit::subpass_description(
    VkPipelineBindPoint bind_point,
    VkAttachmentReference *attachment_reference) {
  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = bind_point;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = attachment_reference;

  return subpass;
}

VkRenderPassCreateInfo VkInit::render_pass_create_info(
    VkAttachmentDescription *attachment,
    VkSubpassDescription *subpass) {
  VkRenderPassCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  info.attachmentCount = 1;
  info.pAttachments = attachment;
  info.subpassCount = 1;
  info.pSubpasses = subpass;

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

VkCommandBufferBeginInfo VkInit::command_buffer_begin_info() {
  VkCommandBufferBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  info.pNext = nullptr;
  info.pInheritanceInfo = nullptr;

  return info;
}

VkRenderPassBeginInfo VkInit::render_pass_begin_info(
    VkRenderPass render_pass,
    VkFramebuffer frame_buffer,
    VkClearValue *clear_value,
    Dimensions dimensions) {
  VkRenderPassBeginInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  info.pNext = nullptr;
  info.renderPass = render_pass;
  info.renderArea.offset.x = 0;
  info.renderArea.offset.y = 0;
  info.renderArea.extent = {dimensions.width, dimensions.height};
  info.framebuffer = frame_buffer;
  info.clearValueCount = 1;
  info.pClearValues = clear_value;

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
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = wait_semaphore;
  info.signalSemaphoreCount = 1;
  info.pSignalSemaphores = signal_semaphore;
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

VkFenceCreateInfo VkInit::fence_create_info() {
  VkFenceCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

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

VkPipelineVertexInputStateCreateInfo VkInit::vertex_input_state_create_info() {
  VkPipelineVertexInputStateCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  info.pNext = nullptr;
  info.vertexBindingDescriptionCount = 0;
  info.vertexAttributeDescriptionCount = 0;

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
  info.cullMode = VK_CULL_MODE_NONE;
  info.frontFace = VK_FRONT_FACE_CLOCKWISE;
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

VkPipelineLayoutCreateInfo VkInit::pipeline_layout_create_info() {
  VkPipelineLayoutCreateInfo info = {};
  info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  info.pNext = nullptr;
  info.flags = 0;
  info.setLayoutCount = 0;
  info.pSetLayouts = nullptr;
  info.pushConstantRangeCount = 0;
  info.pPushConstantRanges = nullptr;

  return info;
}
