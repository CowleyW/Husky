#pragma once

#include "asio/io_context.hpp"
#include "core/types.h"

#include "connection.h"

#include <array>
#include <asio.hpp>

#include <thread>
#include <vector>

namespace Net {

class Server {
public:
  Server(u32 port, u8 max_clients);

  void begin();

private:
  void start_receive();
  void handle_receive(u64 size);

private:
  u32 port;

  u8 max_clients;
  u8 num_connected_clients;
  std::vector<Connection> clients;

  std::unique_ptr<asio::io_context> context;
  asio::ip::udp::socket socket;
  asio::ip::udp::endpoint remote;

  std::thread context_thread;
  std::vector<u8> recv_buf;
};

} // namespace Net
