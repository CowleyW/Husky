#pragma once

#include "util/buf.h"
#include "util/err.h"
#include "util/result.h"

#include <vector>

namespace Net {

enum class MessageType : uint8_t {
  // X -> Y
  // X disconnected
  Disconnected,
  // X pings Y to indicate it is still alive
  Ping,

  // Client -> Server
  // Connection request
  ConnectionRequested,
  // Response to server challenge
  ChallengeResponse,
  // The client inputs
  UserInputs,

  // Server -> Client
  // Accept client connection
  ConnectionAccepted,
  // Deny client connection
  ConnectionDenied,
  // Challenge to confirm client authenticity
  Challenge,
  // The state of all client inputs
  WorldSnapshot
};

struct PacketHeader {
  uint32_t protocol_id;
  uint32_t checksum;

  // Read `MessageHeader::packed_size` for explanation on why this method
  // exists. In this case it is unnecessary since all fields are the same size,
  // but it is best not to rely on that fact when it might change in the future.
  static uint32_t packed_size() { return 8; }

  static Result<PacketHeader> deserialize(const Buf<uint8_t> &buf);
};

struct MessageHeader {
  uint64_t salt;

  // Networking-specific information including packet sequence number
  // and acknowledgments about received packets
  uint32_t sequence_id;
  uint32_t ack;
  uint32_t ack_bitfield;

  // If we need to resend a message we need to include a message id so
  // the receiver can detect if they have received it multiple times
  // and ignore it
  uint32_t message_id;

  // Done with reliability, now we need to handle message type and size
  MessageType message_type;
  uint32_t body_size;

  // Due to C struct alignment we can't use sizeof(MessageHeader) to determine
  // the size of the header in bytes, since message_type will be aligned to 4
  // bytes, but when serializing we want to pack the bytes.
  static uint32_t packed_size() {
    // 1 u64          =  8 bytes
    // 6 u32s = 5 * 4 = 20 bytes
    // 1 MessageType  =  1 byte
    return 29;
  }

  // Serialize the message header into the buffer at the given offset. Returns
  // an error if the buffer does not contain enough space.
  Err serialize_into(std::vector<uint8_t> &buf, uint32_t offset) const;

  static Result<MessageHeader> deserialize(const Buf<uint8_t> &buf);
};

struct Message {
  static constexpr uint32_t CONNECTION_REQUESTED_PADDING = 512;
  static constexpr uint32_t CHALLENGE_RESPONSE_PADDING = 512;

  // The body for a connection accepted message should contain only the new
  // remote id for the client to use (for now)
  static constexpr uint32_t CONNECTION_ACCEPTED_BODY_SIZE = 1;

  MessageHeader header;

  std::vector<uint8_t> body;

  // The minimum possible size for a serialized message to take up. If body = 0
  // bytes then the size is the packed size of the packet header plus the packed
  // size of the message header. Anything smaller can be discarded as it is
  // invalid.
  static uint32_t min_required_size();

  uint32_t packed_size() const;

  // Serialize the complete message into the buffer at the given offset. Returns
  // an error if the buffer does not contain enough space.
  Err serialize_into(std::vector<uint8_t> &buf, uint32_t offset) const;

  static Result<Message> deserialize(const Buf<uint8_t> &buf);
};

Err verify_packet(const Buf<uint8_t> &buf);

} // namespace Net
