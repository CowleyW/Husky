#pragma once

#include "util/err.h"
#include "util/result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace files {

/**
 * Returns a result containing the byte contents of the specified asset file or
 * an error message.
 * @param  path  the file path to be opened. This path is relative to the
 * assets/ directory.
 */
Result<std::vector<uint8_t>> load_file(const std::string &path);

Result<std::vector<uint32_t>> load_spirv_file(const std::string &path);

Result<std::string> load_text_file(const std::string &path);

Err write_file(const std::string &path, std::vector<uint8_t> contents);

Err write_text_file(const std::string &path, std::string_view contents);

Err remove_file(const std::string &path);

} // namespace files
