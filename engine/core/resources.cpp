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
  // 1. Open file specified by the path
  std::string asset_path = std::string(ASSETS_PATH) + path;
  // fopen is 'deprecated' but fopen_s is less portable
  // and provides no meaningful advantages in safety
  std::FILE *file = std::fopen(asset_path.c_str(), "rb");
  if (!file) {
    return Result<std::vector<uint8_t>>::err("Failed to open file");
  }

  // 2. Go to the end of the file to determine file size
  int32_t result;
  result = std::fseek(file, 0, SEEK_END);
  if (result != 0) {
    std::fclose(file);
    return Result<std::vector<uint8_t>>::err("Failed to seek file end");
  }
  uint32_t size = std::ftell(file);
  if (size == -1) {
    std::fclose(file);
    return Result<std::vector<uint8_t>>::err(
        "Failed to determine location in file stream");
  }

  std::vector<uint8_t> data(size, 0);

  // 3. Return to the start of the file and read its contents
  result = std::fseek(file, 0, SEEK_SET);
  if (result != 0) {
    std::fclose(file);
    return Result<std::vector<uint8_t>>::err("Failed to seek to file start");
  }
  std::fread(&data[0], 1, size, file);

  // 4. Close the file
  std::fclose(file);

  return Result<std::vector<uint8_t>>::ok(data);
}
