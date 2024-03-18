#pragma once

#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

#include <vector>

struct InputMap {
  bool press_jump;
  bool press_left;
  bool press_right;

  static uint32_t packed_size() {
    // 3 booleans -> 3 bits
    return 1;
  }

  Err serialize_into(std::vector<uint8_t> &buf, uint32_t offset) const;

  static Result<InputMap> deserialize(const Buf<uint8_t> &buf);
};
