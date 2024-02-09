#pragma once

#include <string_view>

struct Err {
  std::string_view msg;
  bool is_error;

  static Err ok() { return {"", false}; }

  static Err err(std::string_view msg) { return {msg, true}; }
};
