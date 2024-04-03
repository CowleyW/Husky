#pragma once

#include "glm/fwd.hpp"
#include "util/result.h"
#include "vk_types.h"

#include <memory>
#include <utility>
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
  glm::vec2 uvs;

  static VertexInputDescription get_description();
};

struct MeshPushConstant {
  glm::vec4 data;
  glm::mat4 matrix;
};

typedef uint32_t TriMeshHandle;

struct TriMesh {
  std::vector<Vertex> vertices;
  std::string name;

  uint32_t offset;

  uint32_t size() const {
    return this->vertices.size() * sizeof(Vertex);
  }

  static Result<TriMeshHandle> get(const std::string &path);
  static Result<TriMesh *> get(TriMeshHandle handle);

private:
  static Result<TriMeshHandle> load_from_obj(const std::string &obj_path);

  static TriMeshHandle fresh_handle();

private:
  static std::vector<std::pair<TriMeshHandle, TriMesh>> meshes;
};
