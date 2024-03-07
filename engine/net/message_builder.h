#pragma once

#include "message.h"
#include <optional>

namespace Net {

class MessageBuilder {
public:
  MessageBuilder(MessageType type);

  MessageBuilder &with_salt(u64 salt);
  MessageBuilder &with_ids(u32 sequence_id, u32 message_id);
  MessageBuilder &with_acks(u32 ack, u32 ack_bitfield);
  MessageBuilder &with_padding(u32 padding);
  MessageBuilder &with_body(std::vector<u8> body);

  Message build();

  private:
  template <typename T> T unwrap(std::optional<T> opt, std::string_view msg);

private:
  MessageType type;

  std::optional<u64> salt;

  std::optional<u32> sequence_id;

  std::optional<u32> ack;
  std::optional<u32> ack_bitfield;

  std::optional<u32> message_id;

  u32 padding;

  std::vector<u8> body;
};

} // namespace Net
