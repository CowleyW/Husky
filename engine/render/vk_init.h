#pragma once

#include "tri_mesh.h"
#include "vk_types.h"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace VkInit {

VkAttachmentDescription color_attachment(VkFormat format);
VkAttachmentReference color_attachment_ref();

VkAttachmentDescription depth_attachment(VkFormat format);
VkAttachmentReference depth_attachment_ref();

VkSubpassDescription subpass_description(
    VkPipelineBindPoint bind_point,
    VkAttachmentReference *color_attachment_ref,
    VkAttachmentReference *depth_attachment_ref);

VkRenderPassCreateInfo render_pass_create_info(
    std::vector<VkAttachmentDescription> *attachments,
    std::vector<VkSubpassDependency> *dependencies,
    VkSubpassDescription *subpass);

VkRenderPassBeginInfo render_pass_begin_info(
    VkRenderPass render_pass,
    VkFramebuffer frame_buffer,
    std::vector<VkClearValue> *clear_values,
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

VkCommandBufferBeginInfo
command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

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

VkFenceCreateInfo fence_create_info(bool create_signaled = true);

VkImageCreateInfo
image_create_info(VkFormat format, VkImageUsageFlags flags, VkExtent3D extent);

VkImageViewCreateInfo
imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags flags);

VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(
    VkShaderStageFlagBits stage,
    VkShaderModule shader_module);

VkPipelineVertexInputStateCreateInfo
vertex_input_state_create_info(VertexInputDescription *vertex_input = nullptr);

VkPipelineInputAssemblyStateCreateInfo
input_assembly_state_create_info(VkPrimitiveTopology topology);

VkPipelineRasterizationStateCreateInfo
rasterization_state_create_info(VkPolygonMode polygon_mode);

VkPipelineMultisampleStateCreateInfo multisample_state_create_info();

VkPipelineColorBlendAttachmentState color_blend_attachment_state();

VkPipelineLayoutCreateInfo pipeline_layout_create_info(
    std::vector<VkPushConstantRange> *push_constants = nullptr,
    std::vector<VkDescriptorSetLayout> *descriptor_layouts = nullptr);

VkPipelineDepthStencilStateCreateInfo
depth_stencil_create_info(bool depth_test, bool depth_write, VkCompareOp op);

AllocatedBuffer buffer(
    VmaAllocator allocator,
    uint32_t size,
    VkBufferUsageFlags usage,
    VmaMemoryUsage memory_usage);

VkDescriptorSetLayoutBinding descriptor_set_layout_binding(
    VkDescriptorType type,
    VkShaderStageFlags stage,
    uint32_t binding,
    uint32_t count = 1);

VkWriteDescriptorSet write_descriptor_set(
    VkDescriptorType type,
    VkDescriptorSet dest,
    VkDescriptorBufferInfo *info,
    uint32_t binding);

VkSamplerCreateInfo sampler_create_info(
    VkFilter filters,
    VkSamplerAddressMode address_mode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

VkWriteDescriptorSet write_descriptor_image(
    VkDescriptorType type,
    VkDescriptorSet dest,
    VkDescriptorImageInfo *info,
    uint32_t binding,
    uint32_t index);

VkDescriptorSetAllocateInfo descriptor_set_allocate_info(
    VkDescriptorPool &pool,
    VkDescriptorSetLayout &layout);
} // namespace VkInit
