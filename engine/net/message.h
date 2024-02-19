#pragma once

#include "core/types.h"
#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

#include <vector>

namespace Net {

enum class MessageType : u8 {
  ConnectionRequested = 0,
  ConnectionAccepted,
  ConnectionDenied,
  Ping,
  UserInputs
};

struct PacketHeader {
  u32 protocol_id;
  u32 checksum;

  // Read `MessageHeader::packed_size` for explanation on why this method
  // exists. In this case it is unnecessary since all fields are the same size,
  // but it is best not to rely on that fact when it might change in the future.
  static u32 packed_size() { return 8; }

  static Result<PacketHeader> deserialize(const Buf<u8> &buf);
};

struct MessageHeader {
  u32 remote_id;

  // Networking-specific information including packet sequence number
  // and acknowledgments about received packets
  u32 sequence_id;
  u32 ack;
  u32 ack_bitfield;

  // If we need to resend a message we need to include a message id so
  // the receiver can detect if they have received it multiple times
  // and ignore it
  u32 message_id;

  // Done with reliability, now we need to handle message type and size
  MessageType message_type;
  u32 body_size;

  // Due to C struct alignment we can't use sizeof(MessageHeader) to determine
  // the size of the header in bytes, since message_type will be aligned to 4
  // bytes, but when serializing we want to pack the bytes.
  static u32 packed_size() {
    // 6 u32s = 6 * 4 = 24 bytes
    // 1 MessageType  =  1 byte
    return 25;
  }

  // Serialize the message header into the buffer at the given offset. Returns
  // an error if the buffer does not contain enough space.
  Err serialize_into(std::vector<u8> &buf, u32 offset) const;

  static Result<MessageHeader> deserialize(const Buf<u8> &buf);
};

struct Message {
  static constexpr u32 CONNECTION_REQUESTED_PADDING = 512;

  MessageHeader header;

  std::vector<u8> body;

  // The minimum possible size for a serialized message to take up. If body = 0
  // bytes then the size is the packed size of the packet header plus the packed
  // size of the message header. Anything smaller can be discarded as it is
  // invalid.
  static u32 min_required_size();

  u32 packed_size() const;

  // Serialize the complete message into the buffer at the given offset. Returns
  // an error if the buffer does not contain enough space.
  Err serialize_into(std::vector<u8> &buf, u32 offset) const;

  static Result<Message> deserialize(const Buf<u8> &buf);
};

Err verify_packet(const Buf<u8> &buf);

} // namespace Net
