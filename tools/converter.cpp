#include "io/assets.h"
#include "io/files.h"
#include "io/logging.h"
#include "render/tri_mesh.h"
#include "util/serialize.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

#include "stb_image.h"

#include <fstream>
#include <string>

enum class FileType { OBJ, GLTF, GLB, UNKNOWN };

std::string get_filename(const std::string &path) {
  std::size_t sep = path.find_last_of('/');

  if (sep == std::string::npos) {
    return path;
  } else {
    return path.substr(sep + 1);
  }
}

std::string get_filepath(const std::string &path) {
  std::size_t sep = path.find_last_of('/');

  if (sep == std::string::npos) {
    return "";
  } else {
    return path.substr(0, sep);
  }
}

std::string trim_ext(const std::string &file) {
  std::size_t sep = file.find_last_of('.');

  if (sep == std::string::npos) {
    return file;
  } else {
    return file.substr(0, sep);
  }
}

FileType get_file_type(const std::string &file_name) {
  std::size_t sep = file_name.find_last_of('.');

  if (sep == std::string::npos) {
    return FileType::UNKNOWN;
  } else {
    std::string ext = file_name.substr(sep + 1);
    io::debug("File Extension: {}", ext);

    if (ext == "obj") {
      return FileType::OBJ;
    } else if (ext == "glb") {
      return FileType::GLTF;
    } else if (ext == "gltf") {
      return FileType::GLB;
    } else {
      return FileType::UNKNOWN;
    }
  }
}

void save_mesh(
    const std::string &dirpath,
    const std::string &name,
    const TriMesh &mesh,
    const std::string &texture_name) {
  std::string full_path =
      files::full_asset_path(dirpath + "/" + name + ".asset");
  std::ofstream file(full_path, std::ios::binary | std::ios::out);
  if (!file.is_open()) {
    io::error("Failed to write to file {}", full_path);
    return;
  }

  std::vector<uint8_t> count_buf(4);

  // Asset type
  Serialize::serialize_u32((uint32_t)AssetType::Mesh, count_buf, 0);
  file.write((const char *)count_buf.data(), sizeof(AssetType));

  // [size][texture_name]
  Serialize::serialize_u32(texture_name.size(), count_buf, 0);
  file.write((const char *)count_buf.data(), sizeof(uint32_t));
  file.write(texture_name.c_str(), texture_name.size());

  // [num_vertices][vertices]
  Serialize::serialize_u32(mesh.vertices.size(), count_buf, 0);
  file.write((const char *)count_buf.data(), sizeof(uint32_t));
  file.write(
      (const char *)mesh.vertices.data(),
      mesh.vertices.size() * sizeof(Vertex));

  // [num_indices][indices]
  Serialize::serialize_u32(mesh.indices.size(), count_buf, 0);
  file.write((const char *)count_buf.data(), sizeof(uint32_t));
  file.write(
      (const char *)mesh.indices.data(),
      mesh.indices.size() * sizeof(uint32_t));
}

void save_texture(
    const std::string &dirpath,
    const std::string &name,
    uint8_t *data,
    uint32_t width,
    uint32_t height) {
  std::string full_path =
      files::full_asset_path(dirpath + "/" + name + ".asset");
  std::ofstream file(full_path, std::ios::binary | std::ios::out);
  if (!file.is_open()) {
    io::error("Failed to write to file {}", full_path);
    return;
  }

  std::vector<uint8_t> buf(4);
  uint32_t texture_size = width * height * 4;

  // Asset type
  Serialize::serialize_u32((uint32_t)AssetType::Texture, buf, 0);
  file.write((const char *)buf.data(), sizeof(uint32_t));

  // Write the texture data
  Serialize::serialize_u32(width, buf, 0);
  file.write((const char *)buf.data(), sizeof(uint32_t));
  Serialize::serialize_u32(height, buf, 0);
  file.write((const char *)buf.data(), sizeof(uint32_t));
  Serialize::serialize_u32(texture_size, buf, 0);
  file.write((const char *)buf.data(), sizeof(uint32_t));
  file.write((const char *)data, texture_size);

  file.close();
}

void convert_obj(const std::string &path) {
  std::string dirpath = get_filepath(path);
  tinyobj::ObjReaderConfig reader_config = {};
  reader_config.mtl_search_path = files::full_asset_path(dirpath);

  tinyobj::ObjReader reader;
  if (!reader.ParseFromFile(files::full_asset_path(path), reader_config)) {
    io::error("Error Loading Obj: {}", reader.Error());
    return;
  }

  if (!reader.Warning().empty()) {
    io::warn(reader.Warning());
  }

  const tinyobj::attrib_t &attrib = reader.GetAttrib();
  const std::vector<tinyobj::shape_t> &shapes = reader.GetShapes();
  const std::vector<tinyobj::material_t> &materials = reader.GetMaterials();

  if (materials.size() != 1) {
    io::error("Currently only support 1 material per object");
    return;
  }

  TriMesh mesh = {};

  mesh.name = path;
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

  std::string tex_mat_name = get_filename(materials[0].diffuse_texname);
  std::string new_tex_path = dirpath + "/" + trim_ext(tex_mat_name) + ".asset";
  std::string tex_mat_path = dirpath + "/" + tex_mat_name;
  io::debug("Texture Name: {}", tex_mat_name);

  int32_t width, height, channels;
  void *pixels = stbi_load(
      files::full_asset_path(tex_mat_path).c_str(),
      &width,
      &height,
      &channels,
      STBI_rgb_alpha);

  if (!pixels) {
    io::error("Failed to load file {}.", dirpath + tex_mat_name);
    return;
  }

  uint32_t texture_size = width * height * 4;
  std::string tex_name = trim_ext(tex_mat_name);
  save_texture(
      dirpath,
      trim_ext(tex_mat_name),
      (uint8_t *)pixels,
      width,
      height);

  stbi_image_free(pixels);

  std::string name = trim_ext(get_filename(path));
  save_mesh(dirpath, name, mesh, new_tex_path);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    io::error("Usage: converter.exe [asset_name]");
    return 1;
  }

  std::string asset(argv[1]);

  FileType type = get_file_type(asset);

  switch (type) {
  case FileType::OBJ:
    convert_obj(asset);
    break;
  case FileType::UNKNOWN:
    io::error("Unknown filetype for file {}", asset);
    break;
  default:
    io::error("Case not implemented yet.");
    break;
  }

  return 0;
}
