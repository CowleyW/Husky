#include "serialize.h"
#include "io/logging.h"

#include <vector>

u32 Serialize::serialize_u8(u8 value, std::vector<u8> &buf, u32 offset) {
  buf[offset] = value;

  return offset + 1;
}

u32 Serialize::serialize_u16(u16 value, std::vector<u8> &buf, u32 offset) {
  buf[offset] = value >> 8;
  buf[offset + 1] = value;

  return offset + 2;
}

u32 Serialize::serialize_u32(u32 value, std::vector<u8> &buf, u32 offset) {
  buf[offset] = value >> 24;
  buf[offset + 1] = value >> 16;
  buf[offset + 2] = value >> 8;
  buf[offset + 3] = value;

  return offset + 4;
}

u32 Serialize::serialize_u64(u64 value, std::vector<u8> &buf, u32 offset) {
  buf[offset] = value >> 56;
  buf[offset + 1] = value >> 48;
  buf[offset + 2] = value >> 40;
  buf[offset + 3] = value >> 32;
  buf[offset + 4] = value >> 24;
  buf[offset + 5] = value >> 16;
  buf[offset + 6] = value >> 8;
  buf[offset + 7] = value;

  return offset + 8;
}

u8 Serialize::deserialize_u8(const std::vector<u8> &buf, u32 *offset) {
  u8 value = buf[*offset];

  *offset += 1;
  return value;
}

u16 Serialize::deserialize_u16(const std::vector<u8> &buf, u32 *offset) {
  u16 value = 0;

  value += (u16)buf[*offset] << 8;
  value += (u16)buf[*offset + 1];

  *offset += 2;
  return value;
}

u32 Serialize::deserialize_u32(const std::vector<u8> &buf, u32 *offset) {
  u32 value = 0;

  value += (u32)buf[*offset] << 24;
  value += (u32)buf[*offset + 1] << 16;
  value += (u32)buf[*offset + 2] << 8;
  value += (u32)buf[*offset + 3];
  io::info("offset: {}, value: {}", *offset, value);

  *offset += 4;
  return value;
}

u64 Serialize::deserialize_u64(const std::vector<u8> &buf, u32 *offset) {
  u64 value = 0;

  value += (u64)buf[*offset] << 56;
  value += (u64)buf[*offset + 1] << 48;
  value += (u64)buf[*offset + 2] << 40;
  value += (u64)buf[*offset + 3] << 32;
  value += (u64)buf[*offset + 4] << 24;
  value += (u64)buf[*offset + 5] << 16;
  value += (u64)buf[*offset + 6] << 8;
  value += (u64)buf[*offset + 7];

  *offset += 8;
  return value;
}
