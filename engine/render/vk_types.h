#pragma once

#include "io/logging.h"

#include "buffer.h"
#include "render/material.h"
#include "render/tri_mesh.h"

#include <cmath>
#include <tracy/Tracy.hpp>
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
  enum Side { LEFT, RIGHT, TOP, BOTTOM, BACK, FRONT };

  glm::mat4 viewproj;
  glm::vec4 frustums[6];
  uint32_t instance_count;

  CameraData(glm::mat4 viewproj) {
    this->viewproj = viewproj;

    frustums[LEFT].x = viewproj[0].w + viewproj[0].x;
    frustums[LEFT].y = viewproj[1].w + viewproj[1].x;
    frustums[LEFT].z = viewproj[2].w + viewproj[2].x;
    frustums[LEFT].w = viewproj[3].w + viewproj[3].x;

    frustums[RIGHT].x = viewproj[0].w - viewproj[0].x;
    frustums[RIGHT].y = viewproj[1].w - viewproj[1].x;
    frustums[RIGHT].z = viewproj[2].w - viewproj[2].x;
    frustums[RIGHT].w = viewproj[3].w - viewproj[3].x;

    frustums[TOP].x = viewproj[0].w - viewproj[0].y;
    frustums[TOP].y = viewproj[1].w - viewproj[1].y;
    frustums[TOP].z = viewproj[2].w - viewproj[2].y;
    frustums[TOP].w = viewproj[3].w - viewproj[3].y;

    frustums[BOTTOM].x = viewproj[0].w + viewproj[0].y;
    frustums[BOTTOM].y = viewproj[1].w + viewproj[1].y;
    frustums[BOTTOM].z = viewproj[2].w + viewproj[2].y;
    frustums[BOTTOM].w = viewproj[3].w + viewproj[3].y;

    frustums[BACK].x = viewproj[0].w + viewproj[0].z;
    frustums[BACK].y = viewproj[1].w + viewproj[1].z;
    frustums[BACK].z = viewproj[2].w + viewproj[2].z;
    frustums[BACK].w = viewproj[3].w + viewproj[3].z;

    frustums[FRONT].x = viewproj[0].w - viewproj[0].z;
    frustums[FRONT].y = viewproj[1].w - viewproj[1].z;
    frustums[FRONT].z = viewproj[2].w - viewproj[2].z;
    frustums[FRONT].w = viewproj[3].w - viewproj[3].z;

    for (uint32_t i = 0; i < 6; i += 1) {
      glm::vec4 &f = this->frustums[i];

      float length = std::sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
      this->frustums[i] /= length;
    }
  }
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

struct ComputeInstanceData {
  glm::vec3 position;
  MaterialHandle tex_index;
  glm::vec3 rotation;
  uint32_t mesh_index;
  glm::vec3 scale;
  uint32_t _padding2;
};

struct VertexInstanceData {
  glm::mat4 model;
  uint32_t tex_index;
  uint32_t _padding[3];
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

struct DrawStats {
  uint32_t draw_count;
  uint32_t precull_indices;
  uint32_t postcull_indices;
};

enum class ShaderType { Vertex = 0, Fragment };
