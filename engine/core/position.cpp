#include "position.h"

#include "types.h"
#include "util/err.h"
#include "util/serialize.h"

#include <vector>

Err Position::serialize_into(std::vector<u8> &buf, u32 offset) const {
  if (buf.size() < offset + Position::packed_size()) {
    return Err::err("Insufficient space to serialize position");
  }

  offset = Serialize::serialize_float(this->x, buf, offset);
  offset = Serialize::serialize_float(this->y, buf, offset);

  return Err::ok();
}

Result<Position> Position::deserialize(MutBuf<u8> &buf) {
  if (buf.size() < Position::packed_size()) {
    return Result<Position>::err("Buffer is insufficiently sized");
  }

  Position pos = {
      // (x, y)
      Serialize::deserialize_float(buf),
      Serialize::deserialize_float(buf),
  };

  return Result<Position>::ok(pos);
}