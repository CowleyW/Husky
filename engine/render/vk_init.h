#pragma once

#include "vk_types.h"
#include <vulkan/vulkan_core.h>

namespace VkInit {

VkAttachmentDescription color_attachment(VkFormat format);

VkAttachmentReference color_attachment_ref();

VkSubpassDescription subpass_description(
    VkPipelineBindPoint bind_point,
    VkAttachmentReference *attachment_reference);

VkRenderPassCreateInfo render_pass_create_info(
    VkAttachmentDescription *attachment,
    VkSubpassDescription *subpass);

VkRenderPassBeginInfo render_pass_begin_info(
    VkRenderPass render_pass,
    VkFramebuffer frame_buffer,
    VkClearValue *clear_value,
    Dimensions dimensions);

VkFramebufferCreateInfo
frame_buffer_create_info(VkRenderPass render_pass, Dimensions dimensions);

VkCommandPoolCreateInfo command_pool_create_info(
    uint32_t queue_family_index,
    VkCommandPoolCreateFlags flags);

VkCommandBufferAllocateInfo command_buffer_allocate_info(
    VkCommandPool pool,
    uint32_t count,
    VkCommandBufferLevel level);

VkCommandBufferBeginInfo command_buffer_begin_info();

VkSubmitInfo submit_info(
    VkSemaphore *wait_semaphore,
    VkSemaphore *signal_semaphore,
    VkCommandBuffer *cmd,
    VkPipelineStageFlags *flags);

VkPresentInfoKHR present_info(
    VkSwapchainKHR *swapchain,
    VkSemaphore *wait_semaphore,
    uint32_t *image_index);

VkSemaphoreCreateInfo semaphore_create_info();

VkFenceCreateInfo fence_create_info();

} // namespace VkInit
