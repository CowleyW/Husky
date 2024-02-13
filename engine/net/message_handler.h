#pragma once

#include "message.h"

namespace Net {

class MessageHandler {
public:
  virtual void on_connection_requested(const Message &message) {}

  virtual void on_connection_accepted(const Message &message) {}

  virtual void on_connection_denied(const Message &message) {}

  virtual void on_ping(const Message &message) {}
};

} // namespace Net
