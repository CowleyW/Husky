#include "input_map.h"

Err InputMap::serialize_into(std::vector<u8> &buf, u32 offset) const {
  u8 input_mask = 0;
  input_mask += this->press_jump ? 1 : 0;
  input_mask += this->press_left ? 1 << 1 : 0;
  input_mask += this->press_right ? 1 << 2 : 0;
  buf[offset] = input_mask;
  return Err::ok();
}

Result<InputMap> InputMap::deserialize(const Buf<u8> &buf) {
  u8 input_mask = buf.data()[0];
  buf.trim_left(1);

  auto bit_set = [](u8 mask, u8 bit) { return (mask & (1 << bit)) != 0; };

  InputMap map = {bit_set(input_mask, 0), bit_set(input_mask, 1),
                  bit_set(input_mask, 2)};

  return Result<InputMap>::ok(map);
}
