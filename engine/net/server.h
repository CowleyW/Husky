#pragma once

#include "asio/io_context.hpp"
#include "core/types.h"

#include "connection.h"
#include "net/message_handler.h"

#include <asio.hpp>

#include <thread>
#include <vector>

namespace Net {

class Server {
public:
  Server(u32 port, u8 max_clients);

  void begin();

  void register_callbacks(Net::MessageHandler *handler);

  Result<Connection *const> get_client(const asio::ip::udp::endpoint &remote);
  bool has_open_client();
  void accept(const asio::ip::udp::endpoint &remote);
  void deny_connection(const asio::ip::udp::endpoint &remote);

private:
  void start_receive();
  void handle_receive(u64 size);

private:
  u32 port;

  u8 max_clients;
  u8 num_connected_clients;
  std::vector<Connection> clients;

  std::unique_ptr<asio::io_context> context;
  // asio::ip::udp::socket socket;
  Connection listener;
  asio::ip::udp::endpoint remote;

  std::thread context_thread;
  std::vector<u8> recv_buf;

  Net::MessageHandler *handler;
};

} // namespace Net
