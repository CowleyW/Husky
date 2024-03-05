#pragma once

#include "core/types.h"
#include "message_handler.h"
#include "listener.h"
#include "sender.h"

#include <asio.hpp>

#include <array>

namespace Net {

class Client {
public:
  Client(u32 server_port, u32 client_port);

  void register_callbacks(Net::MessageHandler *handler);

  void begin();
  void shutdown();

  void ping_server();
  void send_inputs(const InputMap &inputs);

  void set_remote_id(u32 remote_id);

private:
  std::unique_ptr<asio::io_context> context;

  std::unique_ptr<Listener> listener;
  std::unique_ptr<Sender> sender;

  std::thread context_thread;
  std::array<u8, 1024> recv_buf;

  Net::MessageHandler *handler;
};

} // namespace Net
