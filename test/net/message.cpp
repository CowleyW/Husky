#include "engine/net/message.h"
#include "engine/util/buf.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("MessageHeader transmission works correctly", "[net]") {
  Net::MessageHeader header = {
      0x10,
      0x20,
      0x30,
      0x40,
      0x50,
      Net::MessageType::ConnectionAccepted,
      0x60};

  std::vector<uint8_t> buf(Net::MessageHeader::packed_size());
  Err err = header.serialize_into(buf, 0);
  REQUIRE(!err.is_error);

  Result<Net::MessageHeader> result =
      Net::MessageHeader::deserialize(Buf<uint8_t>(buf));
  REQUIRE(!result.is_error);

  Net::MessageHeader received = result.value;

  REQUIRE(header.salt == received.salt);
  REQUIRE(header.sequence_id == received.sequence_id);
  REQUIRE(header.ack == received.ack);
  REQUIRE(header.ack_bitfield == received.ack_bitfield);
  REQUIRE(header.message_id == received.message_id);
  REQUIRE(header.message_type == received.message_type);
  REQUIRE(header.body_size == received.body_size);
}

TEST_CASE("Message transmission works correctly", "[net]") {
  Net::MessageHeader header = {
      0x10,
      0x20,
      0x30,
      0x40,
      0x50,
      Net::MessageType::ConnectionRequested,
      0x05};

  Net::Message message = {header, {0x60, 0x70, 0x80, 0x90, 0xA0}};

  std::vector<uint8_t> buf(message.packed_size());
  Err err = message.serialize_into(buf, 0);
  REQUIRE(!err.is_error);

  Result<Net::Message> result = Net::Message::deserialize(Buf<uint8_t>(buf));
  REQUIRE(!result.is_error);

  Net::Message received = result.value;

  // Confirm headers match
  {
    Net::MessageHeader new_header = received.header;

    REQUIRE(header.salt == new_header.salt);
    REQUIRE(header.sequence_id == new_header.sequence_id);
    REQUIRE(header.ack == new_header.ack);
    REQUIRE(header.ack_bitfield == new_header.ack_bitfield);
    REQUIRE(header.message_id == new_header.message_id);
    REQUIRE(header.message_type == new_header.message_type);
    REQUIRE(header.body_size == new_header.body_size);
  }

  REQUIRE(message.body == received.body);
}
