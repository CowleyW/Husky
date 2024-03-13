#include "world_state.h"

#include "util/serialize.h"

WorldState::WorldState() : player_positions() {}

WorldState::WorldState(std::vector<std::pair<u8, Position>> player_positions)
    : player_positions(player_positions) {}

Err WorldState::serialize_into(std::vector<u8> &buf, u32 offset) {
  if (buf.size() < offset + this->packed_size()) {
    return Err::err("Insufficient space to serialize position");
  }

  offset = Serialize::serialize_u8(this->player_positions.size(), buf, offset);
  for (auto &pair : this->player_positions) {
    offset = Serialize::serialize_u8(pair.first, buf, offset);

    Err _ = pair.second.serialize_into(buf, offset);
    offset += Position::packed_size();
  }

  return Err::ok();
}

Result<WorldState> WorldState::deserialize(Buf<u8> &buf) {
  MutBuf<u8> mutbuf(buf);
  u8 num_players = Serialize::deserialize_u8(mutbuf);

  if (mutbuf.size() < num_players * WorldState::pair_size()) {
    return Result<WorldState>::err(
        "Insufficient buffer size to deserialize world state");
  }

  std::vector<std::pair<u8, Position>> player_positions(num_players);
  for (u32 i = 0; i < num_players; i += 1) {
    u8 player_index = Serialize::deserialize_u8(mutbuf);
    Position player_position = Position::deserialize(mutbuf).value;

    player_positions.push_back({player_index, player_position});
  }

  return Result<WorldState>::ok(WorldState(player_positions));
}