#include "message.h"

#include "core/def.h"
#include "crypto/checksum.h"
#include "io/logging.h"
#include "util/buf.h"
#include "util/serialize.h"

#include <fmt/core.h>

#include <cstring>

Result<Net::PacketHeader>
Net::PacketHeader::deserialize(const Buf<uint8_t> &buf) {
  if (buf.size() < Net::PacketHeader::packed_size()) {
    return Result<Net::PacketHeader>::err("Buffer is insufficiently sized");
  }

  MutBuf<uint8_t> mutbuf(buf);
  Net::PacketHeader header = {// Protocol ID
                              Serialize::deserialize_u32(mutbuf),
                              // Checksum
                              Serialize::deserialize_u32(mutbuf)};

  return Result<Net::PacketHeader>::ok(header);
}

Err Net::MessageHeader::serialize_into(
    std::vector<uint8_t> &buf,
    uint32_t offset) const {
  if (buf.size() < offset + Net::MessageHeader::packed_size()) {
    return Err::err("Insufficient space to serialize message header");
  }

  offset = Serialize::serialize_u64(this->salt, buf, offset);
  offset = Serialize::serialize_u32(this->sequence_id, buf, offset);
  offset = Serialize::serialize_u32(this->ack, buf, offset);
  offset = Serialize::serialize_u32(this->ack_bitfield, buf, offset);
  offset = Serialize::serialize_u32(this->message_id, buf, offset);
  offset = Serialize::serialize_u8(
      static_cast<uint8_t>(this->message_type),
      buf,
      offset);
  offset = Serialize::serialize_u32(this->body_size, buf, offset);

  return Err::ok();
}

Result<Net::MessageHeader>
Net::MessageHeader::deserialize(const Buf<uint8_t> &buf) {
  if (buf.size() < Net::MessageHeader::packed_size()) {
    return Result<Net::MessageHeader>::err("Buffer is insufficiently sized");
  }

  MutBuf<uint8_t> mutbuf(buf);
  Net::MessageHeader header = {
      // u64 salt
      Serialize::deserialize_u64(mutbuf),
      // u32 sequence_id
      // u32 ack
      // u32 ack_bitfield
      Serialize::deserialize_u32(mutbuf),
      Serialize::deserialize_u32(mutbuf),
      Serialize::deserialize_u32(mutbuf),
      // u32 message_id
      // u8 message_type
      // u32 size
      Serialize::deserialize_u32(mutbuf),
      Serialize::deserialize_enum<MessageType>(mutbuf),
      Serialize::deserialize_u32(mutbuf),
  };

  return Result<Net::MessageHeader>::ok(header);
}

uint32_t Net::Message::min_required_size() {
  return PacketHeader::packed_size() + MessageHeader::packed_size();
}

uint32_t Net::Message::packed_size() const {
  return MessageHeader::packed_size() + this->body.size();
}

Err Net::Message::serialize_into(std::vector<uint8_t> &buf, uint32_t offset)
    const {
  if (buf.size() < offset + this->packed_size()) {
    return Err::err("Insufficient space to serialize message");
  }

  // Safe to ignore the error here since we perform the check above to ensure
  // that there is adequate space
  Err _ = this->header.serialize_into(buf, offset);
  offset += Net::MessageHeader::packed_size();

  // Copy the message body into the buffer
  if (this->body.size() != 0) {
    std::memcpy(&buf[offset], &this->body[0], this->body.size());
  }

  return Err::ok();
}

Result<Net::Message> Net::Message::deserialize(const Buf<uint8_t> &buf) {
  Result<Net::MessageHeader> result = Net::MessageHeader::deserialize(buf);
  if (result.is_error) {
    return Result<Net::Message>::err(result.msg);
  }

  Net::MessageHeader header = result.value;
  if (header.packed_size() + header.body_size != buf.size()) {
    return Result<Net::Message>::err(
        "Buffer size does not match expected size");
  }

  std::vector<uint8_t> body(header.body_size);
  if (body.size() != 0) {
    std::memcpy(&body[0], buf.data() + header.packed_size(), header.body_size);
  }

  Net::Message message = {header, body};

  return Result<Net::Message>::ok(message);
}

Err Net::verify_packet(const Buf<uint8_t> &buf) {
  uint32_t header_size =
      Net::MessageHeader::packed_size() + Net::PacketHeader::packed_size();
  if (buf.size() < header_size) {
    return Err::err("Invalid message, size is too small.");
  }

  Result<Net::PacketHeader> ph_result = Net::PacketHeader::deserialize(buf);
  if (ph_result.is_error) {
    return Err::err("Failed to parse packet header: {}", ph_result.msg);
  }

  Net::PacketHeader packet_header = ph_result.value;
  if (packet_header.protocol_id != NET_PROTOCOL_ID) {
    return Err::err(
        "Invalid protocol id {} (received) != {} (expected)",
        packet_header.protocol_id,
        NET_PROTOCOL_ID);
  }

  uint32_t checksum = Crypto::calculate_checksum(
      buf.trim_left(Net::PacketHeader::packed_size()));
  if (checksum != packet_header.checksum) {
    return Err::err(
        "Failed checksum validation. {} (received) != {} (expected)",
        packet_header.checksum,
        checksum);
  }

  return Err::ok();
}
