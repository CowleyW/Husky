#pragma once

#include "glm/fwd.hpp"
#include "util/result.h"
#include "vk_types.h"

#include <vector>
#include <vulkan/vulkan_core.h>

struct VertexInputDescription {
  std::vector<VkVertexInputBindingDescription> bindings;
  std::vector<VkVertexInputAttributeDescription> attributes;

  VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct TextureCoordinate {
  float u;
  float v;
};

struct Vertex {
  glm::vec3 position;
  glm::vec3 normal;
  glm::vec3 color;
  TextureCoordinate tex;

  static VertexInputDescription get_description();
};

struct MeshPushConstant {
  glm::vec4 data;
  glm::mat4 matrix;
};

struct TriMesh {
  std::vector<Vertex> vertices;
  std::string name;

  AllocatedBuffer vertex_buffer;

  uint32_t size() const {
    return this->vertices.size() * sizeof(Vertex);
  }

  static Result<TriMesh *> get(const std::string &path);

private:
  static Result<TriMesh *> load_from_obj(const std::string &obj_path);

private:
  static std::vector<TriMesh> meshes;
};
