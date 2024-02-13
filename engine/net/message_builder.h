#pragma once

#include "message.h"
#include <optional>

namespace Net {

class MessageBuilder {
public:
  MessageBuilder(MessageType type);

  MessageBuilder &with_ids(u32 remote_id, u32 sequence_id, u32 message_id);
  MessageBuilder &with_acks(u32 ack, u32 ack_bitfield);
  MessageBuilder &with_padding(u32 padding);
  MessageBuilder &with_body(std::vector<u8> body);

  Message build();

private:
  MessageType type;

  std::optional<u32> remote_id;
  std::optional<u32> sequence_id;

  std::optional<u32> ack;
  std::optional<u32> ack_bitfield;

  std::optional<u32> message_id;

  u32 padding;

  std::vector<u8> body;
};

} // namespace Net
