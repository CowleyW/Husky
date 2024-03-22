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
