#pragma once

#include "sender.h"

namespace Net {
class ClientSlot {
public:
  ClientSlot(asio::io_context &context);

  void bind(const asio::ip::udp::endpoint &endpoint, u32 remote_id);
  bool is_connected();
  bool connected_to(const asio::ip::udp::endpoint &endpoint);
  Sender &get_sender();

private:
  bool connected;

  std::unique_ptr<Sender> sender;
};
} // namespace Net