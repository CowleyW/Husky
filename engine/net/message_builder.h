#pragma once

#include "message.h"
#include <optional>

namespace Net {

class MessageBuilder {
public:
  MessageBuilder(MessageType type);

  MessageBuilder &with_salt(uint64_t salt);
  MessageBuilder &with_ids(uint32_t sequence_id, uint32_t message_id);
  MessageBuilder &with_acks(uint32_t ack, uint32_t ack_bitfield);
  MessageBuilder &with_padding(uint32_t padding);
  MessageBuilder &with_body(std::vector<uint8_t> body);

  Message build();

private:
  template <typename T> T unwrap(std::optional<T> opt, std::string_view msg);

private:
  MessageType type;

  std::optional<uint64_t> salt;

  std::optional<uint32_t> sequence_id;

  std::optional<uint32_t> ack;
  std::optional<uint32_t> ack_bitfield;

  std::optional<uint32_t> message_id;

  uint32_t padding;

  std::vector<uint8_t> body;
};

} // namespace Net
