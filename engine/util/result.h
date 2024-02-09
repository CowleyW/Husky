#pragma once

#include <string_view>

template <typename T> struct Result {
  T value;
  std::string_view msg;
  bool is_error;

  static Result<T> ok(T value) { return {value, "", false}; }

  static Result<T> err(std::string_view msg) { return {T(), msg, true}; }
};
