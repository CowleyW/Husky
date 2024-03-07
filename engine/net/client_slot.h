#pragma once

#include "sender.h"

#include <chrono>
#include <queue>

namespace Net {

class ClientSlot {
public:
  ClientSlot(asio::io_context &context);

  void bind(const asio::ip::udp::endpoint &endpoint, u32 remote_id);
  bool is_connected();
  bool connected_to(const asio::ip::udp::endpoint &endpoint);
  bool matches_id(u32 remote);
  Sender &get_sender();
  u32 remote_id();

  std::optional<Message> next_message();
  void add_message(const Message &message);

  void ping();
  void disconnect();
  bool maybe_timeout();

private:
  static constexpr std::chrono::seconds timeout_wait{5};

  bool connected;

  std::queue<Message> message_queue;
  std::unique_ptr<Sender> sender;

  std::chrono::steady_clock::time_point last_message;
};
} // namespace Net