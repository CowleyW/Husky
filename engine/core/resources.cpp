#include "resources.h"

#include "io/logging.h"
#include "util/result.h"

#include <cstdint>
#include <cstdio>
#include <stdio.h>
#include <string>
#include <string_view>
#include <vector>

Result<std::vector<uint8_t>> resources::load_file(const std::string &path) {
  std::string asset_path = std::string(ASSETS_PATH) + path;
  std::FILE *file = std::fopen(asset_path.c_str(), "rb");
  if (!file) {
    return Result<std::vector<uint8_t>>::err("Failed to open file");
  }

  std::fseek(file, 0, SEEK_END);
  uint32_t size = std::ftell(file);

  std::vector<uint8_t> data(size, 0);

  std::fseek(file, 0, SEEK_SET);
  std::fread(&data[0], 1, size, file);

  std::fclose(file);

  return Result<std::vector<uint8_t>>::ok(data);
}
