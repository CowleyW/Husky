#pragma once

#include "util/buf.h"

#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

namespace Serialize {

// Serialize the given uint8_t into the buffer at the given offset.
//
// Returns the new offset.
uint32_t
serialize_u8(uint8_t value, std::vector<uint8_t> &buf, uint32_t offset);

// Serialize the given uint16_t into the buffer at the given offset.
//
// Returns the new offset.
uint32_t
serialize_u16(uint16_t value, std::vector<uint8_t> &buf, uint32_t offset);

// Serialize the given uint32_t into the buffer at the given offset.
//
// Returns the new offset.
uint32_t
serialize_u32(uint32_t value, std::vector<uint8_t> &buf, uint32_t offset);

// Serialize the given uint64_t into the buffer at the given offset.
//
// Returns the new offset.
uint32_t
serialize_u64(uint64_t value, std::vector<uint8_t> &buf, uint32_t offset);

// Serialize the given float into the buffer at the given offset.
//
// Returns the new offset.
uint32_t
serialize_float(float value, std::vector<uint8_t> &buf, uint32_t offset);

// Deserialize the first byte of the buffer into a uint8_t and update the buffer
uint8_t deserialize_u8(MutBuf<uint8_t> &buf);

// Deserialize the first 2 bytes of the buffer into a uint16_t and update the
// buffer
uint16_t deserialize_u16(MutBuf<uint8_t> &buf);
//
// Deserialize the first 4 bytes of the buffer into a uint32_t and update the
// buffer
uint32_t deserialize_u32(MutBuf<uint8_t> &buf);

// Deserialize the first 8 bytes of the buffer into a uint64_t and update the
// buffer
uint64_t deserialize_u64(MutBuf<uint8_t> &buf);

// Deserialize the first 4 bytes of the buffer into a float and update the
// buffer
float deserialize_float(MutBuf<uint8_t> &buf);

std::string deserialize_string(MutBuf<uint8_t> &buf, uint32_t size);

void deserialize_bytes_into(MutBuf<uint8_t> &buf, void *dst, uint32_t size);

template <typename T>
T deserialize_enum(MutBuf<uint8_t> &buf) {
  static_assert(
      std::is_enum<T>::value,
      "Cannot deserialize to a non-enum type.");

  T result;
  std::memcpy(&result, buf.data(), sizeof(T));

  buf.trim_left(sizeof(T));

  return result;
}

} // namespace Serialize
