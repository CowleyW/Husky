#include "engine/util/serialize.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Basic serialization works correctly", "[serialization]") {
  std::vector<u8> buf(15);
  u32 offset = 0;

  offset = Serialize::serialize_u64(0x8877665544332211, buf, offset);
  REQUIRE(offset == 8);

  offset = Serialize::serialize_u32(0xAABBCCDD, buf, offset);
  REQUIRE(offset == 12);

  offset = Serialize::serialize_u16(0xFF99, buf, offset);
  REQUIRE(offset == 14);

  offset = Serialize::serialize_u8(0xEE, buf, offset);
  REQUIRE(offset == 15);

  REQUIRE(buf[0] == 0x88);
  REQUIRE(buf[1] == 0x77);
  REQUIRE(buf[2] == 0x66);
  REQUIRE(buf[3] == 0x55);
  REQUIRE(buf[4] == 0x44);
  REQUIRE(buf[5] == 0x33);
  REQUIRE(buf[6] == 0x22);
  REQUIRE(buf[7] == 0x11);

  REQUIRE(buf[8] == 0xAA);
  REQUIRE(buf[9] == 0xBB);
  REQUIRE(buf[10] == 0xCC);
  REQUIRE(buf[11] == 0xDD);

  REQUIRE(buf[12] == 0xFF);
  REQUIRE(buf[13] == 0x99);

  REQUIRE(buf[14] == 0xEE);
}

TEST_CASE("Basic deserialization works correctly", "[serialization]") {
  std::vector<u8> buf = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88,
                         0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

  u32 offset = 0;

  u64 oct = Serialize::deserialize_u64(buf, &offset);
  REQUIRE(offset == 8);

  u32 quad = Serialize::deserialize_u32(buf, &offset);
  REQUIRE(offset == 12);

  u16 doub = Serialize::deserialize_u16(buf, &offset);
  REQUIRE(offset == 14);

  u8 single = Serialize::deserialize_u8(buf, &offset);
  REQUIRE(offset == 15);

  REQUIRE(oct == 0xFFEEDDCCBBAA9988);
  REQUIRE(quad == 0x77665544);
  REQUIRE(doub == 0x3322);
  REQUIRE(single == 0x11);
}
