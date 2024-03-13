#pragma once

#include "types.h"
#include "util/err.h"
#include "util/result.h"
#include "util/buf.h"

struct Position
{
  float x;
  float y;

  static u32 packed_size() { return 2 * sizeof(float); }

  Err serialize_into(std::vector<u8> &buf, u32 offset);

  static Result<Position> deserialize(MutBuf<u8> &buf);
};