#include "engine/io/files.h"

#include "engine/util/result.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("remove_file works as expected", "[files]") {
  Err err = files::write_text_file("test_RESERVED_TEST_NAME", "whatever");
  REQUIRE(!err.is_error);

  auto result = files::load_file("test_RESERVED_TEST_NAME");
  REQUIRE(!result.is_error);

  err = files::remove_file("test_RESERVED_TEST_NAME");
  REQUIRE(!err.is_error);

  result = files::load_file("test_RESERVED_TEST_NAME");
  REQUIRE(result.is_error);
}

TEST_CASE("remove_file on bad path returns error", "[files]") {
  Err err = files::remove_file("test_RESERVED_TEST_NAME");
  REQUIRE(err.is_error);
}

TEST_CASE("write_text_file and load_text_file work as expected", "[files]") {
  std::string expected = "random stuff in this file";
  Err err = files::write_text_file("test_RESERVED_TEST_NAME", expected);
  REQUIRE(!err.is_error);

  auto result = files::load_text_file("test_RESERVED_TEST_NAME");
  REQUIRE(!result.is_error);

  std::string source = result.value;
  REQUIRE(source.length() == expected.length());
  REQUIRE(source == expected);

  err = files::remove_file("test_RESERVED_TEST_NAME");
  REQUIRE(!err.is_error);
}
