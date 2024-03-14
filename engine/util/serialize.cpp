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

u32 Serialize::serialize_float(float value, std::vector<u8> &buf, u32 offset) {
  union {
    float f;
    u32 val;
  } u = {value};

  return Serialize::serialize_u32(u.val, buf, offset);
}

u8 Serialize::deserialize_u8(MutBuf<u8> &buf) {
  u8 value = buf.data()[0];

  buf.trim_left(1);
  return value;
}

u16 Serialize::deserialize_u16(MutBuf<u8> &buf) {
  u16 value = 0;

  const u8 *data = buf.data();

  value += (u16)data[0] << 8;
  value += (u16)data[1];

  buf.trim_left(2);
  return value;
}

u32 Serialize::deserialize_u32(MutBuf<u8> &buf) {
  u32 value = 0;

  const u8 *data = buf.data();
  value += (u32)data[0] << 24;
  value += (u32)data[1] << 16;
  value += (u32)data[2] << 8;
  value += (u32)data[3];

  buf.trim_left(4);
  return value;
}

u64 Serialize::deserialize_u64(MutBuf<u8> &buf) {
  u64 value = 0;

  const u8 *data = buf.data();
  value += (u64)data[0] << 56;
  value += (u64)data[1] << 48;
  value += (u64)data[2] << 40;
  value += (u64)data[3] << 32;
  value += (u64)data[4] << 24;
  value += (u64)data[5] << 16;
  value += (u64)data[6] << 8;
  value += (u64)data[7];

  buf.trim_left(8);
  return value;
}

float Serialize::deserialize_float(MutBuf<u8> &buf) {
  union {
    u32 val;
    float f;
  } u = {Serialize::deserialize_u32(buf)};

  io::debug("float: {} int: {}", u.val, u.f);
  return u.f;
}
