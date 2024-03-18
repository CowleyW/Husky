#include "message_builder.h"
#include "io/logging.h"
#include "net/message.h"
#include <cstring>
#include <optional>

Net::MessageBuilder::MessageBuilder(Net::MessageType type)
    : type(type), padding(0), body() {}

Net::MessageBuilder &Net::MessageBuilder::with_salt(uint64_t salt) {
  this->salt = salt;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_ids(uint32_t seq_id,
                                                   uint32_t message_id) {
  this->sequence_id = seq_id;
  this->message_id = message_id;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_acks(uint32_t ack,
                                                    uint32_t ack_bitfield) {
  this->ack = ack;
  this->ack_bitfield = ack_bitfield;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_padding(uint32_t padding) {
  this->padding = padding;

  return *this;
}

Net::MessageBuilder &Net::MessageBuilder::with_body(std::vector<uint8_t> body) {
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
  Net::MessageHeader header = {
      this->unwrap(this->salt, "Did not specify salt"),
      this->unwrap(this->sequence_id, "Did not specify sequence_id"),
      this->unwrap(this->ack, "Did not specify ack"),
      this->unwrap(this->ack_bitfield, "Did not specify ack_bitfield"),
      this->unwrap(this->message_id, "Did not specify message_id"),
      this->type,
      (uint32_t)this->body.size() + this->padding};

  std::vector<uint8_t> message_body;
  message_body.resize(header.body_size);
  Net::Message message = {header, message_body};

  if (this->body.size() != 0) {
    std::memcpy(&message.body[0], &this->body[0], this->body.size());
  }

  return message;
}

template <typename T>
T Net::MessageBuilder::unwrap(std::optional<T> opt, std::string_view msg) {
  if (opt.has_value()) {
    return *opt;
  }

  io::warn(msg);
  return T{};
}
