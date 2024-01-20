#pragma once

#include "util/result.h"

#include <cstdint>
#include <string>
#include <vector>

namespace resources {

Result<std::vector<uint8_t>> load_file(const std::string &path);

}
