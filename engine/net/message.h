#pragma once

#include "core/types.h"
#include "util/err.h"

#include <vector>

namespace Net {

// Since minimizing bandwidth is crucial, lets assume that messages
// are small enough such that one message = one packet
struct Header {
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
  u8 message_type;
  u32 size;

  u32 expected_size() const;

  u32 serialize_into(std::vector<u8> &buf) const;

  static Header deserialize_from(const std::vector<u8> &buf, u32 *offset);
};

struct Message {
  Header header{};
  std::vector<u8> body;

  u32 size() const;

  std::vector<u8> serialize() const;

  static Message deserialize_from(const std::vector<u8> &buf);
};

} // namespace Net
