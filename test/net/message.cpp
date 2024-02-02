#include "engine/net/message.h"
#include "io/logging.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Message Serialization and Deserialization works correctly",
          "[net]") {
  Net::Header header = {
      0x4141, 0x4242, 0x4343, 0x4444, 0x45, 0x07,
  };

  Net::Message message = {
      header,
      {0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C},
  };

  std::vector<u8> serialized = message.serialize();

  Net::Message recovered = Net::Message::deserialize_from(serialized);

  {
    auto mh = message.header;
    auto rh = recovered.header;

    REQUIRE(mh.sequence_id == rh.sequence_id);
    REQUIRE(mh.ack == rh.ack);
    REQUIRE(mh.ack_bitfield == rh.ack_bitfield);
    REQUIRE(mh.message_id == rh.message_id);
    REQUIRE(mh.message_type == rh.message_type);
    REQUIRE(mh.size == rh.size);
  }
  REQUIRE(message.body == recovered.body);
}
