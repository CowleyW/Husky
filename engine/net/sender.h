#pragma once

#include "core/types.h"

#include "message_handler.h"
#include "message_builder.h"
#include "io/input_map.h"

#include <asio.hpp>

namespace Net {

class Sender {
public:
  Sender(std::shared_ptr<asio::ip::udp::socket> socket, asio::ip::udp::endpoint endpoint);
  Sender(asio::io_context &context, u32 port, asio::ip::udp::endpoint endpoint);

  void write_connection_requested();
  void write_connection_accepted();
  void write_connection_denied();
  void write_ping();
  void write_user_inputs(const InputMap &inputs);

private:
  MessageBuilder message_scaffold(Net::MessageType type);
  void write_message(const Message &message);

private:
  std::shared_ptr<asio::ip::udp::socket> socket;
  asio::ip::udp::endpoint send_endpoint;

  std::vector<u8> send_buf;

  u32 remote_id;

  u32 sequence_id;
  u32 ack;
  u32 ack_bitfield;

  u32 message_id;
};

} // namespace Net