#pragma once

#include <string_view>

template <typename T> struct Result {
  T value;
  std::string_view msg;
  bool isOk;

  static Result<T> ok(T value) { return {value, "", true}; }

  static Result<T> err(std::string_view msg) { return {(T)0, msg, false}; }
};
