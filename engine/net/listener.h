#pragma once

#include "message_handler.h"

#include <asio.hpp>

namespace Net {

class Listener {
public:
  Listener(std::shared_ptr<asio::ip::udp::socket> socket);
  Listener(asio::io_context &context, uint32_t port);

  void register_callbacks(MessageHandler *handler);

  void listen();

private:
  void handle_receive(uint32_t size);

private:
  std::shared_ptr<asio::ip::udp::socket> socket;
  asio::ip::udp::endpoint recv_endpoint;

  std::vector<uint8_t> recv_buf;

  MessageHandler *handler;
};

} // namespace Net
