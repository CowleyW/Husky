#include "checksum.h"

uint32_t Crypto::calculate_checksum(Buf<uint8_t> buf) {
  return Crypto::calculate_checksum(buf.data(), buf.size());
}

uint32_t Crypto::calculate_checksum(const uint8_t *buf, uint32_t size) {
  static bool initialized_crc32_table = false;
  static uint32_t crc32_table[256];

  // This theoretically might want a mutex to prevent race conditions, but even
  // if the initialization were to happen twice it wouldn't cause an issue as
  // the table is initialized the same way in each instance. So one thread would
  // overwrite the other, but it would overwrite with the same data.
  if (!initialized_crc32_table) {
    initialized_crc32_table = true;
    for (uint32_t i = 0; i < 256; i += 1) {
      uint32_t ch = i;
      uint32_t crc = 0;

      for (uint32_t j = 0; j < 8; j += 1) {
        uint32_t b = (ch ^ crc) & 1;
        crc = crc >> 1;
        if (b == 0) {
          crc = crc ^ 0xEDB88320;
        }
        ch = ch >> 1;
      }
      crc32_table[i] = crc;
    }
  }

  // Calculate the CRC32 checksum based on the (pre-)calculated table
  uint32_t crc = 0xFFFFFFFF;
  for (uint32_t i = 0; i < size; i += 1) {
    uint8_t ch = buf[i];
    uint32_t t = (ch ^ crc) & 0xFF;
    crc = (crc >> 8) ^ crc32_table[t];
  }

  return ~crc;
}
