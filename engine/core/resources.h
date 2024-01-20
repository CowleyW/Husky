#pragma once

#include "util/result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace resources {

/**
 * Returns a result containing the byte contents of the specified asset file or
 * an error message.
 * @param  path  the file path to be opened. This path is relative to the
 * assets/ directory.
 */
Result<std::vector<uint8_t>> load_file(const std::string &path);

} // namespace resources
