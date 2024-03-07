#pragma once

#include "io/logging.h"

#include "asio/ip/udp.hpp"
#include "message.h"

namespace Net {

class MessageHandler {
public:
  void on_message(const Message &message,
                  const asio::ip::udp::endpoint &remote) {
    switch (message.header.message_type) {
    case Net::MessageType::ConnectionRequested:
      this->on_connection_requested(message, remote);
      break;
    case Net::MessageType::ConnectionAccepted:
      this->on_connection_accepted(message, remote);
      break;
    case Net::MessageType::ConnectionDenied:
      this->on_connection_denied(message, remote);
      break;
    case Net::MessageType::Disconnected:
      this->on_disconnected(message, remote);
      break;
    case Net::MessageType::Ping:
      this->on_ping(message, remote);
      break;
    case Net::MessageType::UserInputs:
      this->on_user_inputs(message, remote);
      break;
    default:
      io::error("Got bad message type {}.", (u8)message.header.message_type);
      break;
    }
  }

  virtual void on_connection_requested(const Message &message,
                                       const asio::ip::udp::endpoint &remote) {
    io::error("Unexpected ConnectionRequested message");
  }

  virtual void on_connection_accepted(const Message &message,
                                      const asio::ip::udp::endpoint &remote) {
    io::error("Unexpected ConnectionAccepted message");
  }

  virtual void on_connection_denied(const Message &message,
                                    const asio::ip::udp::endpoint &remote) {
    io::error("Unexpected ConnectionDenied message");
  }

  virtual void on_disconnected(const Message &message,
                               const asio::ip::udp::endpoint &remote) {
    io::error("Unexpected Disconnected message");
  }

  virtual void on_ping(const Message &message,
                       const asio::ip::udp::endpoint &remote) {
    io::error("Unexpected Ping message");
  }

  virtual void on_user_inputs(const Message &message,
                              const asio::ip::udp::endpoint &remote) {
    io::error("Unexpected ClientInputs message");
  }
};

} // namespace Net
