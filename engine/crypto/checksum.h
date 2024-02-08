#pragma once

#include "core/types.h"

#include <vector>

namespace Crypto {

u32 calculate_checksum(std::vector<u8> source);

}
