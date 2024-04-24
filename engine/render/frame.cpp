#include "frame.h"
#include "ecs/components.h"
#include "vk_init.h"

#include "entt/entt.hpp"
#include "imgui_impl_vulkan.h"
#include <imgui.h>
#include <tracy/Tracy.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

void Render::Frame::destroy(VkDevice &device, VmaAllocator &allocator) {
  vkDestroyCommandPool(device, this->graphics_command_pool, nullptr);
  vkDestroyCommandPool(device, this->compute_command_pool, nullptr);
  vkDestroySemaphore(device, this->present_semaphore, nullptr);
  vkDestroySemaphore(device, this->render_semaphore, nullptr);
  vkDestroySemaphore(device, this->compute_semaphore, nullptr);
  vkDestroyFence(device, this->render_fence, nullptr);
  vkDestroyFence(device, this->compute_fence, nullptr);
  this->camera_buffer.destroy(allocator);
  this->vertex_instance_buffer.destroy(allocator);
  this->compute_instance_buffer.destroy(allocator);
  this->indirect_buffer.destroy(allocator);
  this->draw_stats_buffer.destroy(allocator);
}

void Render::Frame::await_compute(VkDevice device) {
  ZoneScoped;
  vkWaitForFences(device, 1, &this->compute_fence, VK_TRUE, 1000000000);
  vkResetFences(device, 1, &this->compute_fence);
}

void Render::Frame::await_render(VkDevice device) {
  ZoneScoped;

  VK_ASSERT(vkWaitForFences(device, 1, &this->render_fence, true, 1000000000));
  VK_ASSERT(vkResetFences(device, 1, &this->render_fence));
}

std::vector<Batch> Render::Frame::copy_renderable_objects(
    entt::registry &registry,
    uint32_t &total_objects,
    VmaAllocator allocator) {
  ZoneScoped;

  TriMeshHandle curr_mesh = TriMesh::NULL_HANDLE;
  MaterialHandle curr_mat = Material::NULL_HANDLE;
  uint32_t indices_size = 0;
  uint32_t first_instance = 0;
  std::vector<Batch> batches = {};
  const auto group = registry.group<Mesh, Transform>();

  void *object_data;
  vmaMapMemory(
      allocator,
      this->compute_instance_buffer.allocation,
      &object_data);

  ComputeInstanceData *ssbo = (ComputeInstanceData *)object_data;
  group.each([&](Mesh &mesh, Transform &transform) {
    ZoneNamedN(__iter_entity, "Iter Entity", true);

    if (!mesh.visible) {
      return;
    }

    if (curr_mat != mesh.material || curr_mesh != mesh.mesh) {
      first_instance = total_objects;
      curr_mat = mesh.material;
      curr_mesh = mesh.mesh;

      Batch batch;
      batch.material = curr_mat;
      batch.mesh = curr_mesh;
      batch.first = first_instance;
      batch.count = 0;
      batches.emplace_back(batch);
    }

    {
      ZoneNamedN(__copy_data, "Copy Matrix", true);
      ssbo[total_objects].position = transform.position;
      ssbo[total_objects].rotation = transform.rotation;
      ssbo[total_objects].scale = transform.scale;
      ssbo[total_objects].tex_index = mesh.material;
      ssbo[total_objects].mesh_index = batches.size() - 1;
    }

    total_objects += 1;
  });
  vmaUnmapMemory(allocator, this->compute_instance_buffer.allocation);

  return batches;
}

void Render::Frame::prepare_indirect_buffer(
    const std::vector<Batch> &batches,
    VmaAllocator allocator) {
  ZoneScoped;

  void *indirect_data;
  vmaMapMemory(allocator, this->indirect_buffer.allocation, &indirect_data);
  VkDrawIndexedIndirectCommand *indirect_buffer =
      (VkDrawIndexedIndirectCommand *)indirect_data;
  for (uint32_t i = 0; i < batches.size(); i += 1) {
    const auto &batch = batches[i];
    ZoneNamedN(__submit_batches, "Submit Batches", true);

    TriMesh *tri_mesh = TriMesh::get(batch.mesh).value;
    Material *mat = Material::get(batch.material).value;

    VkDrawIndexedIndirectCommand cmd = {};
    cmd.firstIndex = tri_mesh->first_index;
    cmd.vertexOffset = tri_mesh->first_vertex;
    cmd.indexCount = tri_mesh->indices.size();
    cmd.firstInstance = batch.first;
    cmd.instanceCount = batch.count;
    indirect_buffer[i] = cmd;
  }
  vmaUnmapMemory(allocator, this->indirect_buffer.allocation);
}

void Render::Frame::prepare_compute_commands(Compute &compute) {
  ZoneScoped;

  VkCommandBufferBeginInfo begin_info = VkInit::command_buffer_begin_info();
  VK_ASSERT(vkBeginCommandBuffer(this->compute_command_buffer, &begin_info));

  vkCmdFillBuffer(
      this->compute_command_buffer,
      this->draw_stats_buffer.buffer,
      0,
      sizeof(DrawStats),
      0);

  VkMemoryBarrier memory_barrier = {};
  memory_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(
      this->compute_command_buffer,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
      0,
      1,
      &memory_barrier,
      0,
      nullptr,
      0,
      nullptr);

  vkCmdBindPipeline(
      this->compute_command_buffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      compute.pipeline);
  vkCmdBindDescriptorSets(
      this->compute_command_buffer,
      VK_PIPELINE_BIND_POINT_COMPUTE,
      compute.pipeline_layout,
      0,
      1,
      &this->compute_descriptor,
      0,
      nullptr);
}

void Render::Frame::submit_compute(Compute &compute, uint32_t total_objects) {
  ZoneScoped;

  uint32_t num_groups = (total_objects / 16) + 1;
  vkCmdDispatch(this->compute_command_buffer, num_groups, 1, 1);

  VK_ASSERT(vkEndCommandBuffer(this->compute_command_buffer));

  VkSubmitInfo submit_info = VkInit::submit_info(
      nullptr,
      0,
      &this->compute_semaphore,
      1,
      &this->compute_command_buffer,
      nullptr);
  VK_ASSERT(vkQueueSubmit(compute.queue, 1, &submit_info, this->compute_fence));
}

void Render::Frame::copy_camera_data(
    VmaAllocator allocator,
    CameraData &camera_data) {
  ZoneScoped;

  void *data;
  vmaMapMemory(allocator, this->camera_buffer.allocation, &data);

  std::memcpy(data, &camera_data, sizeof(CameraData));

  vmaUnmapMemory(allocator, this->camera_buffer.allocation);
}

void Render::Frame::prepare_draw_commands(
    VkRenderPass pass,
    VkPipeline pipeline,
    VkFramebuffer framebuffer,
    Dimensions dimensions) {
  ZoneScoped;

  VK_ASSERT(vkResetCommandBuffer(this->main_command_buffer, 0));

  VkCommandBufferBeginInfo cmd_info = VkInit::command_buffer_begin_info();
  VK_ASSERT(vkBeginCommandBuffer(this->main_command_buffer, &cmd_info));

  std::vector<VkClearValue> clear_values;
  VkClearValue clear_value = {};
  clear_value.color = {{1.0f, 1.0f, 1.0f, 1.0f}};
  VkClearValue depth_value = {};
  depth_value.depthStencil = {1.0f, 0};

  clear_values.push_back(clear_value);
  clear_values.push_back(depth_value);

  VkRenderPassBeginInfo render_pass_info = VkInit::render_pass_begin_info(
      pass,
      framebuffer,
      &clear_values,
      dimensions);

  vkCmdBeginRenderPass(
      this->main_command_buffer,
      &render_pass_info,
      VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(
      this->main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline);

  VkViewport viewport = {};
  viewport.x = 0.0;
  viewport.y = 0.0;
  viewport.width = (float)dimensions.width;
  viewport.height = (float)dimensions.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(this->main_command_buffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent.width = dimensions.width;
  scissor.extent.height = dimensions.height;
  vkCmdSetScissor(this->main_command_buffer, 0, 1, &scissor);
}

void Render::Frame::submit_draw(
    VkSwapchainKHR swapchain,
    VkQueue graphics_queue,
    uint32_t image_index) {
  ZoneScoped;

  ImGui_ImplVulkan_RenderDrawData(
      ImGui::GetDrawData(),
      this->main_command_buffer);

  vkCmdEndRenderPass(this->main_command_buffer);
  VK_ASSERT(vkEndCommandBuffer(this->main_command_buffer));

  std::array<VkPipelineStageFlags, 2> wait_stages = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

  std::array<VkSemaphore, 2> wait_semaphores = {
      this->present_semaphore,
      this->compute_semaphore};
  std::array<VkSemaphore, 1> signal_semaphores = {this->render_semaphore};

  VkSubmitInfo submit_info = VkInit::submit_info(
      wait_semaphores.data(),
      wait_semaphores.size(),
      signal_semaphores.data(),
      signal_semaphores.size(),
      &this->main_command_buffer,
      wait_stages.data());
  VK_ASSERT(vkQueueSubmit(graphics_queue, 1, &submit_info, this->render_fence));

  VkPresentInfoKHR present_info =
      VkInit::present_info(&swapchain, &this->render_semaphore, &image_index);
  VK_ASSERT(vkQueuePresentKHR(graphics_queue, &present_info));
}

void Render::Frame::bind_descriptor_sets(
    VkPipelineLayout layout,
    uint32_t offset) {
  ZoneScoped;

  vkCmdBindDescriptorSets(
      this->main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      layout,
      0,
      1,
      &this->global_descriptor,
      1,
      &offset);

  vkCmdBindDescriptorSets(
      this->main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      layout,
      1,
      1,
      &this->object_descriptor,
      0,
      nullptr);

  vkCmdBindDescriptorSets(
      this->main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      layout,
      2,
      1,
      &this->texture_descriptor,
      0,
      nullptr);
}

void Render::Frame::prepare_graphics_buffers(
    AllocatedBuffer &vertex_buffer,
    AllocatedBuffer &index_buffer,
    uint32_t num_indirect_draws) {
  ZoneScoped;

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      this->main_command_buffer,
      0,
      1,
      &vertex_buffer.buffer,
      &offset);

  vkCmdBindIndexBuffer(
      this->main_command_buffer,
      index_buffer.buffer,
      0,
      VK_INDEX_TYPE_UINT32);

  vkCmdDrawIndexedIndirect(
      this->main_command_buffer,
      this->indirect_buffer.buffer,
      0,
      num_indirect_draws,
      sizeof(VkDrawIndexedIndirectCommand));
}
