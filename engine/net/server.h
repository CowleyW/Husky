#pragma once

#include "asio/io_context.hpp"
#include "core/types.h"

#include "client_slot.h"
#include "listener.h"
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

  std::optional<ClientSlot *const> get_client(const asio::ip::udp::endpoint &remote);
  bool has_open_slot();
  void accept(const asio::ip::udp::endpoint &remote);
  void deny_connection(const asio::ip::udp::endpoint &remote);

private:
  u32 port;

  u8 max_clients;
  u8 num_connected_clients;
  std::vector<ClientSlot> clients;

  std::unique_ptr<asio::io_context> context;
  // asio::ip::udp::socket socket;
  Listener listener;
  Sender denier;
  asio::ip::udp::endpoint remote;

  std::thread context_thread;
  std::vector<u8> recv_buf;

  Net::MessageHandler *handler;
};

} // namespace Net
