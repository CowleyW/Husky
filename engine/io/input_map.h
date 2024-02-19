#pragma once

#include "core/types.h"
#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

#include <vector>

struct InputMap {
  bool press_jump;
  bool press_left;
  bool press_right;

  static u32 packed_size() {
    // 3 booleans -> 3 bits
    return 1;
  }

  Err serialize_into(std::vector<u8> &buf, u32 offset) const;

  static Result<InputMap> deserialize(const Buf<u8> &buf);
};
