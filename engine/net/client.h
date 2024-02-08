#pragma once

#include "core/types.h"

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
  asio::ip::udp::socket socket;
  asio::ip::udp::endpoint server_endpoint;

  std::thread context_thread;
  std::array<u8, 1024> recv_buf;
};

} // namespace Net
