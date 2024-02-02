#pragma once

#include "message.h"

#include <cstddef>
#include <queue>

#include <asio.hpp>

namespace Net {

class MessageQueue {
public:
  MessageQueue(asio::ip::tcp::socket &socket);

  void begin();

  bool has_message();
  Message get_message();

private:
  void begin_enqueue_next();
  void finish_enqueue_next(Header header, std::size_t len);

private:
  asio::ip::tcp::socket &socket;

  std::queue<Message> queue;
  std::vector<u8> buf;
};

} // namespace Net
