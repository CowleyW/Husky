#pragma once

#include "io/logging.h"

#include "buffer.h"
#include "render/material.h"
#include "render/tri_mesh.h"

#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"

#include <cstdint>

#define VK_ASSERT(x)                                                           \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      io::error("[{}:{}] {}", __FILE__, __LINE__, string_VkResult(err));       \
    }                                                                          \
  } while (false)

struct Dimensions {
  uint32_t width;
  uint32_t height;
};

struct CameraData {
  glm::mat4 viewproj;
};

struct SceneData {
  glm::vec4 fog_color;
  glm::vec4 fog_distances;
  glm::vec4 ambient_color;
  glm::vec4 sunlight_direction;
  glm::vec4 sunlight_color;
};

struct AllocatedImage {
  VkImage image;
  VmaAllocation allocation;
};

struct FrameData {
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
  AllocatedBuffer object_buffer;
  AllocatedBuffer indirect_buffer;

  VkDescriptorSet global_descriptor;
  VkDescriptorSet object_descriptor;
  VkDescriptorSet texture_descriptor;
  VkDescriptorSet compute_descriptor;

  void destroy(VkDevice &device, VmaAllocator &allocator) {
    vkDestroyCommandPool(device, this->graphics_command_pool, nullptr);
    vkDestroyCommandPool(device, this->compute_command_pool, nullptr);
    vkDestroySemaphore(device, this->present_semaphore, nullptr);
    vkDestroySemaphore(device, this->render_semaphore, nullptr);
    vkDestroySemaphore(device, this->compute_semaphore, nullptr);
    vkDestroyFence(device, this->render_fence, nullptr);
    vkDestroyFence(device, this->compute_fence, nullptr);
    this->camera_buffer.destroy(allocator);
    this->object_buffer.destroy(allocator);
    this->indirect_buffer.destroy(allocator);
  }
};

struct InstanceData {
  glm::mat4 model;
  MaterialHandle tex_index;
  uint32_t _padding[3];
  // glm::vec3 position;
  // MaterialHandle tex_index;
  // glm::vec3 rotation;
  // TriMeshHandle mesh_index;
  // glm::vec3 scale;
  //
  // uint32_t _padding;
};

struct UploadContext {
  VkFence upload_fence;
  VkCommandPool command_pool;
  VkCommandBuffer command_buffer;
};

struct Texture {
  AllocatedImage image;
  VkImageView image_view;
};

struct Compute {
  VkQueue queue;
  uint32_t queue_family;
  VkDescriptorSetLayout descriptor_layout;
  VkPipelineLayout pipeline_layout;
  VkPipeline pipeline;
};

enum class ShaderType { Vertex = 0, Fragment };
