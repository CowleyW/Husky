#pragma once

#include <fmt/core.h>
#include <string_view>

struct Err {
  std::string msg;
  bool is_error;

  static Err ok() {
    return {"", false};
  }

  static Err err(std::string msg) {
    return {msg, true};
  }

  template <typename... Args>
  static Err err(std::string_view msg, Args &&...args) {
    return err(fmt::format(msg, args...));
  }
};
