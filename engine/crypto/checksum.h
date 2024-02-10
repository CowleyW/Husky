#pragma once

#include "core/types.h"

#include "util/buf.h"

namespace Crypto {

u32 calculate_checksum(Buf<u8> buf);
u32 calculate_checksum(const u8 *buf, u32 size);

} // namespace Crypto
