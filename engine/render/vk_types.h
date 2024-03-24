#pragma once

#include "io/logging.h"

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
      io::error("{}", string_VkResult(err));                                   \
    }                                                                          \
  } while (false)

struct Dimensions {
  uint32_t width;
  uint32_t height;
};

struct CameraData {
  glm::mat4 view;
  glm::mat4 proj;
  glm::mat4 viewproj;
};

struct SceneData {
  glm::vec4 fog_color;
  glm::vec4 fog_distances;
  glm::vec4 ambient_color;
  glm::vec4 sunlight_direction;
  glm::vec4 sunlight_color;
};

struct AllocatedBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;

  static uint32_t padding_size(
      uint32_t original_size,
      const VkPhysicalDeviceProperties &properties) {
    // Efficient function to calculate padding size from:
    // https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
    uint32_t min_alignment = properties.limits.minUniformBufferOffsetAlignment;
    if (min_alignment > 0) {
      return (original_size + min_alignment - 1) & ~(min_alignment - 1);
    } else {
      return original_size;
    }
  }
};

struct AllocatedImage {
  VkImage image;
  VmaAllocation allocation;
};

struct FrameData {
  VkSemaphore present_semaphore;
  VkSemaphore render_semaphore;
  VkFence render_fence;

  VkCommandPool command_pool;
  VkCommandBuffer main_command_buffer;

  AllocatedBuffer camera_buffer;

  VkDescriptorSet global_descriptor;
};

enum class ShaderType { Vertex = 0, Fragment };
