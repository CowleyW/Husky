#pragma once

#include "core/types.h"

#include "message_handler.h"

#include <asio.hpp>

namespace Net {

class Listener {
public:
  Listener(std::shared_ptr<asio::ip::udp::socket> socket);
  Listener(asio::io_context &context, u32 port);

  void register_callbacks(MessageHandler *handler);

  void listen();

private:
  void handle_receive(u32 size);

private:
  std::shared_ptr<asio::ip::udp::socket> socket;
  asio::ip::udp::endpoint recv_endpoint;

  std::vector<u8> recv_buf;

  MessageHandler *handler;
};

} // namespace Net