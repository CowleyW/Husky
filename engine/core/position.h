#pragma once

#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

struct Position {
  float x;
  float y;

  static uint32_t packed_size() {
    return 2 * sizeof(float);
  }

  Err serialize_into(std::vector<uint8_t> &buf, uint32_t offset) const;

  static Result<Position> deserialize(MutBuf<uint8_t> &buf);
};
