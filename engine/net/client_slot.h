#pragma once

#include "sender.h"
#include "types.h"

#include <chrono>
#include <queue>

namespace Net {

class ClientSlot {
public:
  ClientSlot(asio::io_context &context, u8 client_index);

  void bind(const asio::ip::udp::endpoint &endpoint, u64 salt);
  
  bool is_connected();
  bool connected_to(const asio::ip::udp::endpoint &endpoint);
  bool matches_xor_salt(u64 xor_salt);
  bool matches_client_salt(u64 client_salt);
  bool matches_salts(u64 client_salt, u64 server_salt);
  ConnectionStatus connection_status();
  u8 index();

  std::optional<Message> next_message();
  void add_message(const Message &message);

  void accept();
  void send_challenge();
  void ping();
  void disconnect();
  bool maybe_timeout();

private:
  static constexpr std::chrono::seconds timeout_wait{5};

  u8 client_index;

  ConnectionStatus status;

  std::queue<Message> message_queue;
  std::unique_ptr<Sender> sender;

  std::chrono::steady_clock::time_point last_message;
};
} // namespace Net