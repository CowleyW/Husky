#pragma once

#include "position.h"
#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

#include <vector>

class WorldState {
public:
  WorldState();
  WorldState(std::vector<std::pair<uint8_t, Position>> player_positions);

  uint32_t packed_size() const;

  uint32_t player_count() const;
  Result<Position> player_position(uint8_t player_index);

  void remove_player(uint8_t player_index);
  void add_player(uint8_t player_index);
  void transform_player(uint8_t player_index, const Position &transform);

  Err serialize_into(std::vector<uint8_t> &buf, uint32_t offset) const;

  static Result<WorldState> deserialize(Buf<uint8_t> &buf);

private:
  static uint32_t pair_size() {
    return sizeof(uint8_t) + Position::packed_size();
  }

private:
  std::vector<std::pair<uint8_t, Position>> player_positions;
};
