#pragma once

#include <fmt/color.h>
#include <fmt/core.h>

#include <string_view>

namespace io {

template <typename... Args>
void debug(std::string_view msg, Args &&...args) {
  fmt::print(
      fg(fmt::color::light_gray),
      fmt::format("[DEBUG]: {}\n", msg),
      args...);
}

template <typename... Args>
void info(std::string_view msg, Args &&...args) {
  fmt::print(fg(fmt::color::green), fmt::format("[INFO]: {}\n", msg), args...);
}

template <typename... Args>
void warn(std::string_view msg, Args &&...args) {
  fmt::print(fg(fmt::color::gold), fmt::format("[WARN]: {}\n", msg), args...);
}

template <typename... Args>
void error(std::string_view msg, Args &&...args) {
  fmt::print(fg(fmt::color::red), fmt::format("[ERROR]: {}\n", msg), args...);
}

template <typename... Args>
void fatal(std::string_view msg, Args &&...args) {
  fmt::print(
      fg(fmt::color::maroon),
      fmt::format("[FATAL]: {}\n", msg),
      args...);
}

template <typename... Args>
void perf(std::string_view msg, Args &&...args) {
  fmt::print(fg(fmt::color::cyan), fmt::format("[PERF]: {}\n", msg), args...);
}

} // namespace io
