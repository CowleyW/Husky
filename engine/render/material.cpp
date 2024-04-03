#include "material.h"

std::vector<std::pair<MaterialHandle, Material>> materials = {};

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

MaterialHandle Material::fresh_handle() {
  static MaterialHandle next = 0;

  MaterialHandle ret = next;
  next += 1;

  return ret;
}
