#include "engine/net/message.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Message transmission works correctly", "[net]") {
  Net::MessageHeader header = {
      0x10, 0x20, 0x30, 0x40, 0x50, Net::MessageType::ConnectionAccepted, 0x60};

  std::vector<u8> buf(Net::MessageHeader::packed_size());
  Err err = header.serialize_into(buf, 0);
  REQUIRE(!err.is_error);

  Result<Net::MessageHeader> result = Net::MessageHeader::deserialize(buf, 0);
  REQUIRE(!result.is_error);

  Net::MessageHeader received = result.value;

  REQUIRE(header.remote_id == received.remote_id);
  REQUIRE(header.sequence_id == received.sequence_id);
  REQUIRE(header.ack == received.ack);
  REQUIRE(header.ack_bitfield == received.ack_bitfield);
  REQUIRE(header.message_id == received.message_id);
  REQUIRE(header.message_type == received.message_type);
  REQUIRE(header.body_size == received.body_size);
}
