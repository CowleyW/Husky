#include "message.h"

#include "core/types.h"
#include "io/logging.h"
#include "util/serialize.h"
#include <cstring>
#include <vector>

// Temporary solution: header struct is packed when serializing,
// so can't use sizeof(Net::Header) to determine struct size
#define TEMP_HEADER_SIZE 21

u32 Net::Header::expected_size() const { return TEMP_HEADER_SIZE + this->size; }

u32 Net::Header::serialize_into(std::vector<u8> &buf) const {
  // u32 sequence_id
  // u32 ack
  // u32 ack_bitfield
  u32 offset = Serialize::serialize_u32(this->sequence_id, buf, 0);
  offset = Serialize::serialize_u32(this->ack, buf, offset);
  offset = Serialize::serialize_u32(this->ack_bitfield, buf, offset);

  // u32 message_id
  // u8 message_type
  // u32 size
  offset = Serialize::serialize_u32(this->message_id, buf, offset);
  offset = Serialize::serialize_u8(this->message_type, buf, offset);
  offset = Serialize::serialize_u32(this->size, buf, offset);

  return offset;
}

Net::Header Net::Header::deserialize_from(const std::vector<u8> &buf,
                                          u32 *offset) {
  return Net::Header{
      // u32 sequence_id
      // u32 ack
      // u32 ack_bitfield
      Serialize::deserialize_u32(buf, offset),
      Serialize::deserialize_u32(buf, offset),
      Serialize::deserialize_u32(buf, offset),
      // u32 message_id
      // u8 message_type
      // u32 size
      Serialize::deserialize_u32(buf, offset),
      Serialize::deserialize_u8(buf, offset),
      Serialize::deserialize_u32(buf, offset),
  };
}

u32 Net::Message::size() const { return TEMP_HEADER_SIZE + this->body.size(); }

std::vector<u8> Net::Message::serialize() const {
  std::vector<u8> buf(this->size());

  u32 offset = this->header.serialize_into(buf);

  // Copy the message body into buffer
  std::memcpy(&buf[offset], &this->body[0], this->body.size());

  return buf;
}

Net::Message Net::Message::deserialize_from(const std::vector<u8> &buf) {
  u32 offset = 0;
  Net::Header header = Net::Header::deserialize_from(buf, &offset);

  std::vector<u8> body(buf.begin() + offset, buf.end());

  return Net::Message{
      header,
      body,
  };
}
