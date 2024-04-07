#include "serialize.h"
#include "io/logging.h"

#include <cstring>
#include <vector>

uint32_t Serialize::serialize_u8(
    uint8_t value,
    std::vector<uint8_t> &buf,
    uint32_t offset) {
  buf[offset] = value;

  return offset + 1;
}

uint32_t Serialize::serialize_u16(
    uint16_t value,
    std::vector<uint8_t> &buf,
    uint32_t offset) {
  buf[offset] = value >> 8;
  buf[offset + 1] = value;

  return offset + 2;
}

uint32_t Serialize::serialize_u32(
    uint32_t value,
    std::vector<uint8_t> &buf,
    uint32_t offset) {
  buf[offset] = value >> 24;
  buf[offset + 1] = value >> 16;
  buf[offset + 2] = value >> 8;
  buf[offset + 3] = value;

  return offset + 4;
}

uint32_t Serialize::serialize_u64(
    uint64_t value,
    std::vector<uint8_t> &buf,
    uint32_t offset) {
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

uint32_t Serialize::serialize_float(
    float value,
    std::vector<uint8_t> &buf,
    uint32_t offset) {
  union {
    float f;
    uint32_t val;
  } u = {value};

  return Serialize::serialize_u32(u.val, buf, offset);
}

uint8_t Serialize::deserialize_u8(MutBuf<uint8_t> &buf) {
  uint8_t value = buf.data()[0];

  buf.trim_left(1);
  return value;
}

uint16_t Serialize::deserialize_u16(MutBuf<uint8_t> &buf) {
  uint16_t value = 0;

  const uint8_t *data = buf.data();

  value += (uint16_t)data[0] << 8;
  value += (uint16_t)data[1];

  buf.trim_left(2);
  return value;
}

uint32_t Serialize::deserialize_u32(MutBuf<uint8_t> &buf) {
  uint32_t value = 0;

  const uint8_t *data = buf.data();
  value += (uint32_t)data[0] << 24;
  value += (uint32_t)data[1] << 16;
  value += (uint32_t)data[2] << 8;
  value += (uint32_t)data[3];

  buf.trim_left(4);
  return value;
}

uint64_t Serialize::deserialize_u64(MutBuf<uint8_t> &buf) {
  uint64_t value = 0;

  const uint8_t *data = buf.data();
  value += (uint64_t)data[0] << 56;
  value += (uint64_t)data[1] << 48;
  value += (uint64_t)data[2] << 40;
  value += (uint64_t)data[3] << 32;
  value += (uint64_t)data[4] << 24;
  value += (uint64_t)data[5] << 16;
  value += (uint64_t)data[6] << 8;
  value += (uint64_t)data[7];

  buf.trim_left(8);
  return value;
}

float Serialize::deserialize_float(MutBuf<uint8_t> &buf) {
  union {
    uint32_t val;
    float f;
  } u = {Serialize::deserialize_u32(buf)};

  io::debug("float: {} int: {}", u.val, u.f);
  return u.f;
}

std::string Serialize::deserialize_string(MutBuf<uint8_t> &buf, uint32_t size) {
  std::string str;

  if (size == 0) {
    return "";
  }

  str.resize(size);
  std::memcpy(str.data(), buf.data(), size);

  buf.trim_left(size);
  return str;
}

void Serialize::deserialize_bytes_into(
    MutBuf<uint8_t> &buf,
    void *dst,
    uint32_t size) {
  if (size == 0) {
    return;
  }

  std::memcpy(dst, buf.data(), size);

  buf.trim_left(size);
}
