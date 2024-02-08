#pragma once

#include "core/types.h"
#include <vector>

namespace Net {

enum class MessageType : u8 {
  ConnectionRequested = 0,
  ConnectionAccepted,
  ConnectionDenied
};

struct PacketHeader {
  u32 protocol_id;
  u32 checksum;
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
};

struct Message {
  MessageHeader header;

  std::vector<u8> body;
};

} // namespace Net
