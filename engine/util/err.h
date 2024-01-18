#pragma once

#include <string_view>

struct Err {
  std::string_view msg;
  bool isOk;

  static Err ok() {
    return { "", true };
  }

  static Err err(std::string_view msg) {
    return { msg, false };
  }
};
