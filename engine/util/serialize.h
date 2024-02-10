#pragma once

#include "core/types.h"
#include "util/buf.h"

#include <cstring>
#include <type_traits>
#include <vector>

namespace Serialize {

// Serialize the given u8 into the buffer at the given offset.
//
// Returns the new offset.
u32 serialize_u8(u8 value, std::vector<u8> &buf, u32 offset);

// Serialize the given u16 into the buffer at the given offset.
//
// Returns the new offset.
u32 serialize_u16(u16 value, std::vector<u8> &buf, u32 offset);

// Serialize the given u32 into the buffer at the given offset.
//
// Returns the new offset.
u32 serialize_u32(u32 value, std::vector<u8> &buf, u32 offset);

// Serialize the given u64 into the buffer at the given offset.
//
// Returns the new offset.
u32 serialize_u64(u64 value, std::vector<u8> &buf, u32 offset);

// Deserialize the first byte at the given offset into the buffer and update the
// offset to the first unread byte
u8 deserialize_u8(MutBuf<u8> &buf);
//
// Deserialize the first 2 bytes at the given offset into the buffer and update
// the offset to the first unread byte
u16 deserialize_u16(MutBuf<u8> &buf);
//
// Deserialize the first 4 bytes at the given offset into the buffer and update
// the offset to the first unread byte
u32 deserialize_u32(MutBuf<u8> &buf);

// Deserialize the first 8 bytes at the given offset into the buffer and update
// the offset to the first unread byte
u64 deserialize_u64(MutBuf<u8> &buf);

template <typename T> T deserialize_enum(MutBuf<u8> &buf) {
  static_assert(std::is_enum<T>::value,
                "Cannot deserialize to a non-enum type.");

  T result;
  std::memcpy(&result, buf.data(), sizeof(T));

  buf.trim_left(sizeof(T));

  return result;
}

} // namespace Serialize
