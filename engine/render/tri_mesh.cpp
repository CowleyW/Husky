#include "tri_mesh.h"
#include "io/files.h"

#include "io/logging.h"
#include "tiny_obj_loader.h"

#include <cstddef>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

bool Vertex::operator==(const Vertex &other) const {
  return this->position == other.position && this->color == other.color &&
         this->normal == other.normal && this->uvs == other.uvs;
}

std::vector<std::pair<TriMeshHandle, TriMesh>> TriMesh::meshes =
    std::vector<std::pair<TriMeshHandle, TriMesh>>();

VertexInputDescription Vertex::get_description() {
  VertexInputDescription description;

  VkVertexInputBindingDescription main_binding = {};
  main_binding.binding = 0;
  main_binding.stride = sizeof(Vertex);
  main_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  description.bindings.push_back(main_binding);

  VkVertexInputAttributeDescription position_attrib = {};
  position_attrib.binding = 0;
  position_attrib.location = 0;
  position_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_attrib.offset = offsetof(Vertex, position);

  VkVertexInputAttributeDescription normal_attrib = {};
  normal_attrib.binding = 0;
  normal_attrib.location = 1;
  normal_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
  normal_attrib.offset = offsetof(Vertex, normal);

  VkVertexInputAttributeDescription color_attrib = {};
  color_attrib.binding = 0;
  color_attrib.location = 2;
  color_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
  color_attrib.offset = offsetof(Vertex, color);

  VkVertexInputAttributeDescription texture_attrib = {};
  texture_attrib.binding = 0;
  texture_attrib.location = 3;
  texture_attrib.format = VK_FORMAT_R32G32_SFLOAT;
  texture_attrib.offset = offsetof(Vertex, uvs);

  description.attributes.push_back(position_attrib);
  description.attributes.push_back(normal_attrib);
  description.attributes.push_back(color_attrib);
  description.attributes.push_back(texture_attrib);

  return description;
}

Result<TriMeshHandle> TriMesh::get(const std::string &path) {
  for (auto &pair : TriMesh::meshes) {
    auto handle = pair.first;
    auto &mesh = pair.second;
    if (mesh.name == path) {
      return Result<TriMeshHandle>::ok(handle);
    }
  }

  auto res_handle = TriMesh::load_from_obj(path);
  return res_handle;
}

Result<TriMesh *> TriMesh::get(TriMeshHandle handle) {
  for (auto &pair : TriMesh::meshes) {
    if (pair.first == handle) {
      TriMesh &mesh = pair.second;
      return Result<TriMesh *>::ok(&pair.second);
    }
  }

  return Result<TriMesh *>::err("Could not find Mesh with the given handle");
}

Result<TriMeshHandle> TriMesh::load_from_obj(const std::string &obj_path) {
  std::string full_obj_path = files::full_asset_path(obj_path);

  tinyobj::ObjReaderConfig reader_config = {};
  reader_config.mtl_search_path = ASSETS_PATH "objs/";

  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(full_obj_path, reader_config)) {
    return Result<TriMeshHandle>::err("Error Loading Obj: {}", reader.Error());
  }

  if (!reader.Warning().empty()) {
    io::warn(reader.Warning());
  }

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();
  const std::vector<tinyobj::material_t> &materials = reader.GetMaterials();

  TriMesh::meshes.push_back({TriMesh::fresh_handle(), {}});
  auto &pair = TriMesh::meshes.back();

  TriMeshHandle handle = pair.first;
  TriMesh &mesh = pair.second;

  mesh.name = obj_path;
  mesh.vertices = std::vector<Vertex>();
  mesh.indices = std::vector<uint32_t>();

  // Our approach to loading .obj files
  // -> For each shape
  //    -> for each face
  //       -> add triangle positions
  //       -> add triangle normals

  std::unordered_map<Vertex, uint32_t> vertices = {};

  // Loop over shapes
  for (uint32_t s = 0; s < shapes.size(); s += 1) {
    // Loop over faces
    uint32_t offset = 0;
    for (uint32_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f += 1) {
      for (uint32_t v = 0; v < 3; v += 1) {
        tinyobj::index_t index = shapes[s].mesh.indices[offset + v];

        Vertex vert = {};
        vert.position.x = attrib.vertices[3 * index.vertex_index + 0];
        vert.position.y = attrib.vertices[3 * index.vertex_index + 1];
        vert.position.z = attrib.vertices[3 * index.vertex_index + 2];

        if (index.normal_index >= 0) {
          vert.normal.x = attrib.normals[3 * index.normal_index + 0];
          vert.normal.y = attrib.normals[3 * index.normal_index + 1];
          vert.normal.z = attrib.normals[3 * index.normal_index + 2];
        }

        if (index.texcoord_index >= 0) {
          vert.uvs.x = attrib.texcoords[2 * index.texcoord_index + 0];
          vert.uvs.y = 1 - attrib.texcoords[2 * index.texcoord_index + 1];
        }

        vert.color = vert.normal;

        if (vertices.count(vert) == 0) {
          vertices[vert] = vertices.size();
          mesh.vertices.push_back(vert);
        }

        mesh.indices.push_back(vertices[vert]);
      }
      offset += 3;
    }
  }

  io::debug(
      "Vertices: {}, Indices: {}",
      mesh.vertices.size(),
      mesh.indices.size());

  return Result<TriMeshHandle>::ok(handle);
}

TriMeshHandle TriMesh::fresh_handle() {
  static TriMeshHandle handle = 0;

  TriMeshHandle ret = handle;
  handle += 1;

  return ret;
}
