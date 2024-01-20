#include "engine/core/resources.h"

#include "engine/util/result.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("remove_file works as expected", "[resources]") {
  Err err = resources::write_text_file("test_RESERVED_TEST_NAME", "whatever");
  REQUIRE(err.isOk);

  auto result = resources::load_file("test_RESERVED_TEST_NAME");
  REQUIRE(result.isOk);

  err = resources::remove_file("test_RESERVED_TEST_NAME");
  REQUIRE(err.isOk);

  result = resources::load_file("test_RESERVED_TEST_NAME");
  REQUIRE(!result.isOk);
}

TEST_CASE("remove_file on bad path returns error", "[resources]") {
  Err err = resources::remove_file("test_RESERVED_TEST_NAME");
  REQUIRE(!err.isOk);
}

TEST_CASE("write_text_file and load_text_file work as expected",
          "[resources]") {
  std::string expected = "random stuff in this file";
  Err err = resources::write_text_file("test_RESERVED_TEST_NAME", expected);
  REQUIRE(err.isOk);

  auto result = resources::load_text_file("test_RESERVED_TEST_NAME");
  REQUIRE(result.isOk);

  std::string source = result.value;
  REQUIRE(source.length() == expected.length());
  REQUIRE(source == expected);

  err = resources::remove_file("test_RESERVED_TEST_NAME");
  REQUIRE(err.isOk);
}
