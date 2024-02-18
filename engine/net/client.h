#pragma once

#include "core/types.h"
#include "net/connection.h"
#include "net/message_handler.h"

#include <asio.hpp>

#include <array>

namespace Net {

class Client {
public:
  Client(u32 server_port, u32 client_port);

  void register_callbacks(Net::MessageHandler *handler);

  void begin();
  void shutdown();

private:
  void start_receive();
  void handle_receive(u64 size);

private:
  std::unique_ptr<asio::io_context> context;
  asio::ip::udp::endpoint server_endpoint;

  Connection server_connection;

  std::thread context_thread;
  std::array<u8, 1024> recv_buf;

  Net::MessageHandler *handler;

  asio::ip::udp::endpoint remote;
};

} // namespace Net
