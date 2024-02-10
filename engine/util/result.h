#pragma once

#include <fmt/core.h>

template <typename T> struct Result {
  T value;
  std::string msg;
  bool is_error;

  static Result<T> ok(T value) { return {value, "", false}; }

  static Result<T> err(std::string msg) { return {T(), msg, true}; }

  template <typename... Args>
  static Result<T> err(std::string msg, Args &&...args) {
    return err(fmt::format(msg, args...));
  }
};
