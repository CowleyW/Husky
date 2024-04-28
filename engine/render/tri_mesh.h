#pragma once

#include "bounding_boxes.h"
#include "material.h"
#include "util/result.h"
#include "vertex.h"
#include "vk_types.h"

#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "glm/fwd.hpp"

struct MeshPushConstant {
  glm::vec4 data;
  glm::mat4 matrix;
};

typedef uint32_t TriMeshHandle;

struct TriMesh {
  static constexpr TriMeshHandle NULL_HANDLE = UINT32_MAX;

public:
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::string name;
  std::string texture_name;

  uint32_t first_vertex;
  uint32_t first_index;

public:
  uint32_t vertex_buffer_size() const {
    return this->vertices.size() * sizeof(Vertex);
  }

  uint32_t index_buffer_size() const {
    return this->indices.size() * sizeof(uint32_t);
  }

  AABB aabb() const;

  static Result<TriMeshHandle> get(const std::string &path);
  static Result<TriMesh *> get(TriMeshHandle handle);
  static Result<std::string> get_texture_name(TriMeshHandle handle);

private:
  static Result<TriMeshHandle> load_from_obj(const std::string &obj_path);
  static Result<TriMeshHandle> load_from_asset(const std::string &asset_path);

  static TriMeshHandle fresh_handle();

private:
  static std::vector<std::pair<TriMeshHandle, TriMesh>> meshes;
};

struct Batch {
  MaterialHandle material;
  TriMeshHandle mesh;
  uint32_t first;
  uint32_t count;
};
