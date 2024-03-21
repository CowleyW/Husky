#include "input_map.h"

#include "io/logging.h"

Err InputMap::serialize_into(std::vector<uint8_t> &buf, uint32_t offset) const {
  uint8_t input_mask = 0;
  input_mask += this->press_jump ? 1 : 0;
  input_mask += this->press_left ? 1 << 1 : 0;
  input_mask += this->press_right ? 1 << 2 : 0;
  buf[offset] = input_mask;
  return Err::ok();
}

Result<InputMap> InputMap::deserialize(const Buf<uint8_t> &buf) {
  uint8_t input_mask = buf.data()[0];
  buf.trim_left(1);

  auto bit_set = [](uint8_t mask, uint8_t bit) {
    return (mask & (1 << bit)) != 0;
  };

  InputMap map = {
      bit_set(input_mask, 0),
      bit_set(input_mask, 1),
      bit_set(input_mask, 2)};

  return Result<InputMap>::ok(map);
}
