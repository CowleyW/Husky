#pragma once

#include "glm/fwd.hpp"
#include "util/result.h"
#include "vk_types.h"

#include <glm/gtx/hash.hpp>
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

  bool operator==(const Vertex &other) const;

  static VertexInputDescription get_description();
};

// Inject Vertex hashing into std namespace
namespace std {
template <>
struct hash<Vertex> {
  size_t operator()(Vertex const &vertex) const {
    size_t pos = hash<glm::vec3>()(vertex.position);
    size_t nrm = hash<glm::vec3>()(vertex.normal);
    size_t clr = hash<glm::vec3>()(vertex.color);
    size_t uvs = hash<glm::vec2>()(vertex.uvs);

    return ((pos ^ (nrm << 1) >> 1) ^ (clr << 1) >> 1) ^ (uvs << 1);
  }
};
} // namespace std

struct MeshPushConstant {
  glm::vec4 data;
  glm::mat4 matrix;
};

typedef uint32_t TriMeshHandle;

struct TriMesh {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::string name;

  uint32_t vertices_offset;
  uint32_t indices_offset;

  uint32_t vertex_buffer_size() const {
    return this->vertices.size() * sizeof(Vertex);
  }

  uint32_t index_buffer_size() const {
    return this->indices.size() * sizeof(uint32_t);
  }

  static Result<TriMeshHandle> get(const std::string &path);
  static Result<TriMesh *> get(TriMeshHandle handle);

private:
  static Result<TriMeshHandle> load_from_obj(const std::string &obj_path);

  static TriMeshHandle fresh_handle();

private:
  static std::vector<std::pair<TriMeshHandle, TriMesh>> meshes;
};
