#pragma once

#include "vk_types.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/glm.hpp"
#include <glm/gtx/hash.hpp>

#include <utility>
#include <vector>

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
