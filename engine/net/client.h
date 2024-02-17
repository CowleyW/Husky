#pragma once

#include "core/types.h"
#include "net/connection.h"

#include <asio.hpp>

#include <array>

namespace Net {

class Client {
public:
  Client(u32 port);

  void begin();
  void shutdown();

private:
  std::unique_ptr<asio::io_context> context;
  asio::ip::udp::endpoint server_endpoint;

  Connection server_connection;

  std::thread context_thread;
  std::array<u8, 1024> recv_buf;
};

} // namespace Net
