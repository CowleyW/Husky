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

  u32 packed_size() const;

  u32 player_count() const;
  Result<Position> player_position(u8 player_index);

  void remove_player(u8 player_index);
  void add_player(u8 player_index);
  void transform_player(u8 player_index, const Position &transform);

  Err serialize_into(std::vector<u8> &buf, u32 offset) const;

  static Result<WorldState> deserialize(Buf<u8> &buf);

private:
  static u32 pair_size() { return sizeof(u8) + Position::packed_size(); }

private:
  std::vector<std::pair<u8, Position>> player_positions;
};