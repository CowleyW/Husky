#pragma once

#include "sender.h"
#include "types.h"

#include <chrono>
#include <queue>

namespace Net {

class ClientSlot {
public:
  ClientSlot(asio::io_context &context, uint8_t client_index);

  void bind(const asio::ip::udp::endpoint &endpoint, uint64_t salt);

  bool is_connected();
  bool connected_to(const asio::ip::udp::endpoint &endpoint);
  bool matches_xor_salt(uint64_t xor_salt);
  bool matches_client_salt(uint64_t client_salt);
  bool matches_salts(uint64_t client_salt, uint64_t server_salt);
  ConnectionStatus connection_status();
  uint8_t index();

  std::optional<Message> next_message();
  void add_message(const Message &message);

  void accept();
  void send_challenge();
  void ping();
  void send_world_state(const std::vector<uint8_t> &serialized_state);
  void disconnect();
  bool maybe_timeout();

private:
  static constexpr std::chrono::seconds timeout_wait{5};

  uint8_t client_index;

  ConnectionStatus status;

  std::queue<Message> message_queue;
  std::unique_ptr<Sender> sender;

  std::chrono::steady_clock::time_point last_message;
};
} // namespace Net
