#pragma once

#include "util/result.h"
#include "vk_types.h"

#include "glm/glm.hpp"

#include <vector>
#include <vulkan/vulkan_core.h>

struct VertexInputDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;

  VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;

  static VertexInputDescription get_description();
};

struct MeshPushConstant {
  glm::vec4 data;
  glm::mat4 matrix;
};

struct Mesh {
  std::vector<Vertex> vertices;

  AllocatedBuffer vertex_buffer;

  uint32_t size() {
    return this->vertices.size() * sizeof(Vertex);
  }

  static Result<Mesh> load_from_obj(const std::string &obj_path);
};
