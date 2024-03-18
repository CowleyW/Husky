#pragma once

#include "util/buf.h"

namespace Crypto {

uint32_t calculate_checksum(Buf<uint8_t> buf);
uint32_t calculate_checksum(const uint8_t *buf, uint32_t size);

} // namespace Crypto
