#include "files.h"

#include "util/result.h"

#include <cstdio>
#include <stdio.h>
#include <string>
#include <string_view>
#include <vector>

Result<std::vector<uint8_t>> files::load_file(const std::string &path) {
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

Result<std::string> files::load_text_file(const std::string &path) {
  auto bytes_result = files::load_file(path);
  if (bytes_result.is_error) {
    return Result<std::string>::err(bytes_result.msg);
  } else {
    std::string value(bytes_result.value.begin(), bytes_result.value.end());
    return Result<std::string>::ok(value);
  }
}

Err files::write_file(const std::string &path, std::vector<uint8_t> contents) {
  std::string asset_path = std::string(ASSETS_PATH) + path;

  std::FILE *file = std::fopen(asset_path.c_str(), "wb");
  if (!file) {
    return Err::err("Failed to open file");
  }

  std::fwrite(contents.data(), 1, contents.size(), file);
  std::fclose(file);
  return Err::ok();
}

Err files::write_text_file(const std::string &path, std::string_view contents) {
  std::vector<uint8_t> data(contents.data(),
                            contents.data() + contents.length());
  return files::write_file(path, data);
}

Err files::remove_file(const std::string &path) {
  std::string asset_path = std::string(ASSETS_PATH) + path;

  int32_t result = std::remove(asset_path.c_str());
  if (result != 0) {
    return Err::err("Failed to delete file");
  }

  return Err::ok();
}
