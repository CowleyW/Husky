#pragma once

#include "core/types.h"

#include <vector>

namespace Serialize {

u32 serialize_u8(u8 value, std::vector<u8> &buf, u32 offset);
u32 serialize_u16(u16 value, std::vector<u8> &buf, u32 offset);
u32 serialize_u32(u32 value, std::vector<u8> &buf, u32 offset);
u32 serialize_u64(u64 value, std::vector<u8> &buf, u32 offset);

u8 deserialize_u8(const std::vector<u8> &buf, u32 *offset);
u16 deserialize_u16(const std::vector<u8> &buf, u32 *offset);
u32 deserialize_u32(const std::vector<u8> &buf, u32 *offset);
u64 deserialize_u64(const std::vector<u8> &buf, u32 *offset);

} // namespace Serialize
