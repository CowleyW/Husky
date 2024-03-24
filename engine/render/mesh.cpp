#include "mesh.h"
#include "io/files.h"

#include "io/logging.h"
#include "tiny_obj_loader.h"

#include <cstddef>
#include <vector>
#include <vulkan/vulkan_core.h>

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

  description.attributes.push_back(position_attrib);
  description.attributes.push_back(normal_attrib);
  description.attributes.push_back(color_attrib);

  return description;
}

Result<Mesh> Mesh::load_from_obj(const std::string &obj_path) {
  std::string full_obj_path = files::full_asset_path(obj_path);

  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string warn;
  std::string err;

  tinyobj::LoadObj(
      &attrib,
      &shapes,
      &materials,
      &warn,
      &err,
      full_obj_path.c_str(),
      nullptr);
  if (!warn.empty()) {
    io::warn(warn);
  }

  if (!err.empty()) {
    return Result<Mesh>::err(err);
  }

  Mesh mesh = {};

  // Our approach to loading .obj files
  // -> For each shape
  //    -> for each face
  //       -> add triangle positions
  //       -> add triangle normals

  // Loop over shapes
  for (uint32_t s = 0; s < shapes.size(); s += 1) {
    // Loop over faces
    uint32_t offset = 0;
    for (uint32_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f += 1) {
      for (uint32_t v = 0; v < 3; v += 1) {
        tinyobj::index_t index = shapes[s].mesh.indices[offset + v];

        Vertex vert;
        vert.position.x = attrib.vertices[3 * index.vertex_index + 0];
        vert.position.y = attrib.vertices[3 * index.vertex_index + 1];
        vert.position.z = attrib.vertices[3 * index.vertex_index + 2];

        vert.normal.x = attrib.normals[3 * index.normal_index + 0];
        vert.normal.y = attrib.normals[3 * index.normal_index + 1];
        vert.normal.z = attrib.normals[3 * index.normal_index + 2];

        vert.color = vert.normal;

        mesh.vertices.push_back(vert);
      }
      offset += 3;
    }
  }

  return Result<Mesh>::ok(mesh);
}
