#pragma once

#include <ostream>
#include <string_view>

namespace logging {

void debug(std::ostream &stream, std::string_view msg);
void info(std::ostream &stream, std::string_view msg);
void warn(std::ostream &stream, std::string_view msg);
void error(std::ostream &stream, std::string_view msg);
void fatal(std::ostream &stream, std::string_view msg);

void console_debug(std::string_view msg);
void console_info(std::string_view msg);
void console_warn(std::string_view msg);
void console_error(std::string_view msg);
void console_fatal(std::string_view msg);

}
