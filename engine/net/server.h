#pragma once

#include "connection.h"
#include "core/types.h"

#include <asio.hpp>

#include <memory>
#include <vector>

namespace Net {

class Server {
public:
  Server(u32 port);

  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  void begin();

private:
  void start_accept();
  void handle_accept(std::shared_ptr<Connection> connection);

private:
  std::shared_ptr<asio::io_context> context;
  asio::ip::tcp::acceptor acceptor;

  std::thread context_thread;

  u32 port;

  std::vector<std::shared_ptr<Connection>> clients;
};

} // namespace Net
