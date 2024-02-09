#include "message.h"

#include "core/types.h"
#include "io/logging.h"
#include "util/serialize.h"
#include <cstring>

Err Net::MessageHeader::serialize_into(std::vector<u8> &buf, u32 offset) {
  if (buf.size() < offset + Net::MessageHeader::packed_size()) {
    return Err::err("Insufficient space to serialize message header");
  }

  offset = Serialize::serialize_u32(this->remote_id, buf, offset);
  offset = Serialize::serialize_u32(this->sequence_id, buf, offset);
  offset = Serialize::serialize_u32(this->ack, buf, offset);
  offset = Serialize::serialize_u32(this->ack_bitfield, buf, offset);
  offset = Serialize::serialize_u32(this->message_id, buf, offset);
  offset =
      Serialize::serialize_u8(static_cast<u8>(this->message_type), buf, offset);
  offset = Serialize::serialize_u32(this->body_size, buf, offset);

  return Err::ok();
}

Result<Net::MessageHeader>
Net::MessageHeader::deserialize(const std::vector<u8> &buf, u32 offset) {
  if (buf.size() < offset + Net::MessageHeader::packed_size()) {
    return Result<Net::MessageHeader>::err("Buffer is insufficiently sized");
  }

  io::info("offset: {}", offset);
  Net::MessageHeader header = {
      // u32 remote_id
      Serialize::deserialize_u32(buf, &offset),
      // u32 sequence_id
      // u32 ack
      // u32 ack_bitfield
      Serialize::deserialize_u32(buf, &offset),
      Serialize::deserialize_u32(buf, &offset),
      Serialize::deserialize_u32(buf, &offset),
      // u32 message_id
      // u8 message_type
      // u32 size
      Serialize::deserialize_u32(buf, &offset),
      Serialize::deserialize_enum<MessageType>(buf, &offset),
      Serialize::deserialize_u32(buf, &offset),
  };

  return Result<Net::MessageHeader>::ok(header);
}

u32 Net::Message::min_required_size() {
  return PacketHeader::packed_size() + MessageHeader::packed_size();
}

u32 Net::Message::packed_size() {
  return MessageHeader::packed_size() + this->body.size();
}

Err Net::Message::serialize_into(std::vector<u8> &buf, u32 offset) {
  if (buf.size() <= offset + this->packed_size()) {
    return Err::err("Insufficient space to serialize message");
  }

  // Safe to ignore the error here since we perform the check above to ensure
  // that there is adequate space
  Err _ = this->header.serialize_into(buf, offset);
  offset += Net::MessageHeader::packed_size();

  // Copy the message body into the buffer
  std::memcpy(&buf[offset], &this->body[0], this->body.size());

  return Err::ok();
}

Result<Net::Message> Net::Message::deserialize(const std::vector<u8> &buf,
                                               u32 offset) {
  Result<Net::MessageHeader> result =
      Net::MessageHeader::deserialize(buf, offset);
  if (result.is_error) {
    return Result<Net::Message>::err(result.msg);
  }

  Net::MessageHeader header = result.value;
  u32 body_begin = offset + header.packed_size();
  if (body_begin + header.body_size != buf.size()) {
    return Result<Net::Message>::err(
        "Buffer size does not match expected size");
  }

  std::vector<u8> body(header.body_size);
  std::memcpy(&body[0], &buf[body_begin], header.body_size);

  Net::Message message = {header, body};

  return Result<Net::Message>::ok(message);
}
