#include "material.h"

std::vector<std::pair<MaterialHandle, Material>> Material::materials = {};

Result<Material *> Material::get(MaterialHandle handle) {
  for (auto &pair : Material::materials) {
    if (pair.first == handle) {
      Material &mesh = pair.second;
      return Result<Material *>::ok(&pair.second);
    }
  }

  return Result<Material *>::err(
      "Could not find Material with the given handle");
}

Result<MaterialHandle> Material::get(const std::string &path) {
  for (auto &pair : Material::materials) {
    auto handle = pair.first;
    auto *mat = &pair.second;

    if (mat->material_name == path) {
      return Result<MaterialHandle>::ok(handle);
    }
  }

  Material::materials.push_back({Material::fresh_handle(), {}});
  auto &pair = Material::materials.back();

  MaterialHandle handle = pair.first;
  Material &material = pair.second;
  material.material_name = path;

  return Result<MaterialHandle>::ok(handle);
}

MaterialHandle Material::fresh_handle() {
  static MaterialHandle next = 0;

  MaterialHandle ret = next;
  next += 1;

  return ret;
}
