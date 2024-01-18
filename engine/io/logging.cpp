#include "logging.h"

#include <iostream>
#include <ostream>
#include <string_view>

namespace logging {

void debug(std::ostream &stream, std::string_view msg) {
  stream << "[DEBUG]: " << msg << "\n";
}

void info(std::ostream &stream, std::string_view msg) {
  stream << "[INFO]: " << msg << "\n";
}

void warn(std::ostream &stream, std::string_view msg) {
  stream << "[WARN]: " << msg << "\n";
}

void error(std::ostream &stream, std::string_view msg) {
  stream << "[ERROR]: " << msg << "\n";
}

void fatal(std::ostream &stream, std::string_view msg) {
  stream << "[FATAL]: " << msg << "\n";
}

void console_debug(std::string_view msg) { debug(std::cerr, msg); }
void console_info(std::string_view msg) { info(std::cerr, msg); }
void console_warn(std::string_view msg) { warn(std::cerr, msg); }
void console_error(std::string_view msg) { error(std::cerr, msg); }
void console_fatal(std::string_view msg) { fatal(std::cerr, msg); }

} // namespace logging
