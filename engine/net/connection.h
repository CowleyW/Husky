#pragma once

#include "message.h"
#include "message_queue.h"

#include "asio/error_code.hpp"
#include <asio.hpp>

namespace Net {

class Connection {
public:
  Connection(asio::io_context &context);

  void write_message(Message &message);

  void begin_queue();
  void shutdown();

  bool has_message();
  Message get_message();

public:
  asio::ip::tcp::socket socket;

private:
  void handle_write(const asio::error_code &err, u64 len);

private:
  MessageQueue queue;
};

} // namespace Net
