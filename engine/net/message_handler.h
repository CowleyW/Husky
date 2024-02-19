#pragma once

#include "asio/ip/udp.hpp"
#include "message.h"

namespace Net {

class MessageHandler {
public:
  virtual void on_connection_requested(const Message &message,
                                       const asio::ip::udp::endpoint &remote) {}

  virtual void on_connection_accepted(const Message &message) {}

  virtual void on_connection_denied(const Message &message) {}

  virtual void on_ping(const Message &message) {}

  virtual void on_user_inputs(const Message &message) {}
};

} // namespace Net
