#pragma once

#include "entt/entity/fwd.hpp"
#include "vk_types.h"
#include <vulkan/vulkan_core.h>

namespace Render {
struct Frame {
  VkSemaphore present_semaphore;
  VkSemaphore render_semaphore;
  VkSemaphore compute_semaphore;

  VkFence render_fence;
  VkFence compute_fence;

  VkCommandPool graphics_command_pool;
  VkCommandPool compute_command_pool;
  VkCommandBuffer main_command_buffer;
  VkCommandBuffer compute_command_buffer;

  AllocatedBuffer camera_buffer;
  AllocatedBuffer vertex_instance_buffer;
  AllocatedBuffer compute_instance_buffer;
  AllocatedBuffer indirect_buffer;
  AllocatedBuffer draw_stats_buffer;

  VkDescriptorSet global_descriptor;
  VkDescriptorSet object_descriptor;
  VkDescriptorSet texture_descriptor;
  VkDescriptorSet compute_descriptor;

  void destroy(VkDevice &device, VmaAllocator &allocator);

  void await_compute(VkDevice device);

  void await_render(VkDevice device);

  std::vector<Batch> copy_renderable_objects(
      entt::registry &registry,
      uint32_t &total_objects,
      VmaAllocator allocator);

  void prepare_indirect_buffer(
      const std::vector<Batch> &batches,
      VmaAllocator allocator);

  void prepare_compute_commands(Compute &compute);

  void submit_compute(Compute &compute, uint32_t total_objects);

  void copy_camera_data(VmaAllocator allocator, CameraData &camera_data);

  void prepare_draw_commands(
      VkRenderPass pass,
      VkPipeline pipeline,
      VkFramebuffer framebuffer,
      Dimensions dimensions);

  void submit_draw(
      VkSwapchainKHR swapchain,
      VkQueue graphics_queue,
      uint32_t image_index);

  void bind_descriptor_sets(VkPipelineLayout layout, uint32_t offset);
  void prepare_graphics_buffers(
      AllocatedBuffer &vertex_buffer,
      AllocatedBuffer &index_buffer,
      uint32_t num_indirect_draws);
};
} // namespace Render
