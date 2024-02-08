#include "checksum.h"

#include "core/types.h"

#include <vector>

u32 Crypto::calculate_checksum(std::vector<u8> source) {
  static bool initialized_crc32_table = false;
  static u32 crc32_table[256];

  // This theoretically might want a mutex to prevent race conditions, but even
  // if the initialization were to happen twice it wouldn't cause an issue as
  // the table is initialized the same way in each instance. So one thread would
  // overwrite the other, but it would overwrite with the same data.
  if (!initialized_crc32_table) {
    initialized_crc32_table = true;
    for (u32 i = 0; i < 256; i += 1) {
      u32 ch = i;
      u32 crc = 0;

      for (u32 j = 0; j < 8; j += 1) {
        u32 b = (ch ^ crc) & 1;
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
  u32 crc = 0xFFFFFFFF;
  for (u32 i = 0; i < source.size(); i += 1) {
    u8 ch = source[i];
    u32 t = (ch ^ crc) & 0xFF;
    crc = (crc >> 8) ^ crc32_table[t];
  }

  return ~crc;
}
