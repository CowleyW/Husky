#pragma once

#include "position.h"
#include "types.h"
#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

#include <vector>

class WorldState {
public:
  WorldState();
  WorldState(std::vector<std::pair<u8, Position>> player_positions);

  u32 packed_size() {
    // 1 byte for the number of players, rest to store positions vec
    return sizeof(u8) + this->player_positions.size() * WorldState::pair_size();
  }

  Err serialize_into(std::vector<u8> &buf, u32 offset);

  static Result<WorldState> deserialize(Buf<u8> &buf);

private:
  static u32 pair_size() { return sizeof(u8) + Position::packed_size(); }

private:
  std::vector<std::pair<u8, Position>> player_positions;
};