#include "message_builder.h"
#include "io/logging.h"
#include "net/message.h"
#include <cstring>
#include <optional>

Net::MessageBuilder::MessageBuilder(Net::MessageType type)
    : type(type), padding(0), body() {}

Net::MessageBuilder &Net::MessageBuilder::with_ids(u32 remote_id, u32 seq_id,
                                                   u32 message_id) {
  this->remote_id = remote_id;
  this->sequence_id = seq_id;
  this->message_id = message_id;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_acks(u32 ack, u32 ack_bitfield) {
  this->ack = ack;
  this->ack_bitfield = ack_bitfield;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_padding(u32 padding) {
  this->padding = padding;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_body(std::vector<u8> body) {
  // I'm not thrilled with how this part of message_builder is handled at the
  // moment, as it seems to be performing wasteful copies. This is code that in
  // the future I would like to redo, but it likely doesn't have a huge impact,
  // especially relative to the actual impact of network latency. i.e. the time
  // cost of operating on and handling messages on the CPU is dominated by the
  // time cost of the network. That said, it is not an excuse to write poor code
  this->body = body;

  return *this;
}

Net::Message Net::MessageBuilder::build() {
  auto value_or_0 = [](std::optional<u32> opt, const std::string &msg) {
    if (opt.has_value()) {
      return *opt;
    }

    io::warn(msg);
    return (u32)0;
  };

  Net::MessageHeader header = {
      value_or_0(this->remote_id, "Did not specify remote_id"),
      value_or_0(this->sequence_id, "Did not specify sequence_id"),
      value_or_0(this->ack, "Did not specify ack"),
      value_or_0(this->ack_bitfield, "Did not specify ack_bitfield"),
      value_or_0(this->message_id, "Did not specify message_id"),
      this->type,
      (u32)this->body.size() + this->padding};

  Net::Message message = {header, std::vector<u8>(header.body_size)};

  std::memcpy(&message.body[0], &this->body[0], this->body.size());

  return message;
}
