#include "world_state.h"

#include "util/serialize.h"

WorldState::WorldState() : player_positions() {
}

WorldState::WorldState(
    std::vector<std::pair<uint8_t, Position>> player_positions)
    : player_positions(player_positions) {
}

uint32_t WorldState::packed_size() const {
  // 1 byte for the number of players, rest to store positions vec
  return sizeof(uint8_t) +
         this->player_positions.size() * WorldState::pair_size();
}

uint32_t WorldState::player_count() const {
  return this->player_positions.size();
}

Result<Position> WorldState::player_position(uint8_t player_index) {
  for (auto &pair : this->player_positions) {
    if (pair.first == player_index) {
      return Result<Position>::ok(pair.second);
    }
  }

  return Result<Position>::err("No player of the given index.");
}

void WorldState::remove_player(uint8_t player_index) {
  for (uint32_t i = 0; i < this->player_positions.size(); i += 1) {
    if (this->player_positions[i].first == player_index) {
      this->player_positions.erase(
          this->player_positions.begin() + i,
          this->player_positions.begin() + i + 1);
    }
  }
}

void WorldState::add_player(uint8_t player_index) {
  this->player_positions.push_back({player_index, {0.0, 0.0}});
}

void WorldState::transform_player(
    uint8_t player_index,
    const Position &transform) {
  for (auto &pair : this->player_positions) {
    if (pair.first == player_index) {
      pair.second.x += transform.x;
      pair.second.y += transform.y;
    }
  }
}

Err WorldState::serialize_into(std::vector<uint8_t> &buf, uint32_t offset)
    const {
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

Result<WorldState> WorldState::deserialize(Buf<uint8_t> &buf) {
  MutBuf<uint8_t> mutbuf(buf);
  uint8_t num_players = Serialize::deserialize_u8(mutbuf);

  if (mutbuf.size() < num_players * WorldState::pair_size()) {
    return Result<WorldState>::err(
        "Insufficient buffer size to deserialize world state");
  }

  std::vector<std::pair<uint8_t, Position>> player_positions(num_players);
  for (uint32_t i = 0; i < num_players; i += 1) {
    uint8_t player_index = Serialize::deserialize_u8(mutbuf);
    Position player_position = Position::deserialize(mutbuf).value;

    player_positions[i] = {player_index, player_position};
  }

  return Result<WorldState>::ok(WorldState(player_positions));
}
