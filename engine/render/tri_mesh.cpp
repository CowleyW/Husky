#include "tri_mesh.h"
#include "core/perf.h"
#include "io/assets.h"
#include "io/files.h"

#include "io/logging.h"
#include "render/vk_types.h"
#include "tiny_obj_loader.h"
#include "util/buf.h"
#include "util/serialize.h"

#include <cstddef>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

std::vector<std::pair<TriMeshHandle, TriMesh>> TriMesh::meshes = {};

AABB TriMesh::aabb() const {
  return AABB(this->vertices);
}

Result<TriMeshHandle> TriMesh::get(const std::string &path) {
  for (auto &pair : TriMesh::meshes) {
    auto handle = pair.first;
    auto &mesh = pair.second;
    if (mesh.name == path) {
      return Result<TriMeshHandle>::ok(handle);
    }
  }

  PERF_BEGIN(LoadAsset);
  auto res_handle = TriMesh::load_from_asset(path);
  PERF_END(LoadAsset);
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

Result<std::string> TriMesh::get_texture_name(TriMeshHandle handle) {
  auto maybe = TriMesh::get(handle);

  if (maybe.is_error) {
    return Result<std::string>::err(maybe.msg);
  } else {
    return Result<std::string>::ok(maybe.value->texture_name);
  }
}

Result<TriMeshHandle> TriMesh::load_from_asset(const std::string &asset_path) {
  auto res_file = files::load_file(asset_path);

  if (res_file.is_error) {
    return Result<TriMeshHandle>::err(res_file.msg);
  }

  std::vector<uint8_t> &buf = res_file.value;
  MutBuf mutbuf(res_file.value);
  AssetType asset_type = (AssetType)Serialize::deserialize_u32(mutbuf);
  if (asset_type != AssetType::Mesh) {
    io::debug("type: {}", (uint32_t)asset_type);
    return Result<TriMeshHandle>::err("Invalid Asset Type");
  }

  // TODO: Add some better error checking here. We assume that nobody has
  // modified the generated files from the converter

  TriMesh::meshes.push_back({TriMesh::fresh_handle(), {}});
  auto &pair = TriMesh::meshes.back();

  TriMeshHandle handle = pair.first;
  TriMesh &mesh = pair.second;

  uint32_t tex_name_size = Serialize::deserialize_u32(mutbuf);
  mesh.texture_name = Serialize::deserialize_string(mutbuf, tex_name_size);

  uint32_t vertices_count = Serialize::deserialize_u32(mutbuf);
  mesh.vertices.resize(vertices_count);
  Serialize::deserialize_bytes_into(
      mutbuf,
      mesh.vertices.data(),
      vertices_count * sizeof(Vertex));

  uint32_t indices_count = Serialize::deserialize_u32(mutbuf);
  mesh.indices.resize(indices_count);
  Serialize::deserialize_bytes_into(
      mutbuf,
      mesh.indices.data(),
      indices_count * sizeof(uint32_t));

  mesh.name = asset_path;

  return Result<TriMeshHandle>::ok(handle);
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
