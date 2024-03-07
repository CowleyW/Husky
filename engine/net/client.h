#pragma once

#include "core/types.h"
#include "listener.h"
#include "message_handler.h"
#include "sender.h"
#include "types.h"

#include <asio.hpp>

#include <array>
#include <chrono>
#include <queue>

namespace Net {
using namespace std::literals::chrono_literals;

class Client : MessageHandler {
public:
  Client(u32 server_port, u32 client_port);

  void begin();
  void shutdown();
  bool maybe_timeout();

  void ping_server();
  void send_inputs(const InputMap &inputs);
  void disconnect();

  bool is_connected();
  ConnectionStatus connection_status();

  std::optional<Message> next_message();

public:
  void on_connection_accepted(const Message &message,
                              const asio::ip::udp::endpoint &remote) override;

  void on_connection_denied(const Message &message,
                            const asio::ip::udp::endpoint &remote) override;

  void on_ping(const Message &message,
               const asio::ip::udp::endpoint &remote) override;

private:
  void add_message(const Message &message);

private:
  static constexpr std::chrono::seconds timeout_wait{5};

  ConnectionStatus status;
  std::chrono::steady_clock::time_point last_message;

  std::unique_ptr<asio::io_context> context;

  std::unique_ptr<Listener> listener;
  std::unique_ptr<Sender> sender;

  std::thread context_thread;
  std::array<u8, 1024> recv_buf;

  std::queue<Net::Message> messages;
};

} // namespace Net
