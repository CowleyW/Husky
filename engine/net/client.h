#pragma once

#include "connection.h"
#include "core/types.h"

#include <asio.hpp>

namespace Net {

class Client {
public:
  Client(u32 port);

  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;

  void begin();
  void shutdown();

  bool has_message();
  Message get_message();

private:
  std::shared_ptr<asio::io_context> context;

  std::shared_ptr<Connection> server_connection;

  std::thread context_thread;

  u32 port;
};

} // namespace Net
