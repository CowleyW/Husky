#pragma once

#include "message.h"
#include "message_queue.h"

#include "asio/error_code.hpp"
#include <asio.hpp>
#include <cstddef>

namespace Net {

class Connection {
public:
  Connection(asio::io_context &context);

  void write_message(Message &message);

  void begin_queue();

  bool has_message();
  Message get_message();

public:
  asio::ip::tcp::socket socket;

private:
  void handle_write(const asio::error_code &err, std::size_t len);

private:
  MessageQueue queue;
};

} // namespace Net
