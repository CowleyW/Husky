#include "src/io/logging.h"

#include <sstream>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("logging::debug writes to stream correctly", "[logging]") {
  std::stringstream ss;

  logging::debug(ss, "Writing debug output!");

  REQUIRE(ss.str() == "[DEBUG]: Writing debug output!\n");
}

TEST_CASE("multiple logs work correctly", "[logging]") {
  std::stringstream ss;

  logging::debug(ss, "Writing debug output!");
  logging::info(ss, "Writing some info.");
  logging::warn(ss, "Writing a warning...");
  logging::error(ss, "Writing an error.");
  logging::fatal(ss, "It all falls down...");

  REQUIRE(ss.str() == "[DEBUG]: Writing debug output!\n"
                      "[INFO]: Writing some info.\n"
                      "[WARN]: Writing a warning...\n"
                      "[ERROR]: Writing an error.\n"
                      "[FATAL]: It all falls down...\n");
}
