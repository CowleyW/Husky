#pragma once

#include "asio/io_context.hpp"
#include "io/input_map.h"
#include "message.h"
#include "message_handler.h"

#include "core/types.h"
#include "net/message_builder.h"
#include "util/err.h"

#include <asio.hpp>

namespace Net {

class Connection {
public:
  Connection(asio::io_context &context);
  Connection(asio::io_context &context, u32 port);

  void free(); 
  Err bind(const asio::ip::udp::endpoint &remote);
  void register_callbacks(MessageHandler *handler);

  void listen();

  bool is_connected();
  bool matches_id(u32 id);
  bool matches_remote(const asio::ip::udp::endpoint &send_endpoint);
  void write_message(const Message &message);

  void write_connection_requested();
  void write_connection_accepted();
  void write_connection_denied();
  void write_ping();
  void write_user_inputs(const InputMap &inputs);

private:
  void handle_receive(u32 size);
  Net::MessageBuilder message_scaffold(Net::MessageType type);

private:
  bool connected;

  // The remote (client) endpoint that we write to
  asio::ip::udp::endpoint send_endpoint;
  asio::ip::udp::endpoint recv_endpoint;
  asio::ip::udp::socket socket;

  std::vector<u8> send_buf;
  std::vector<u8> recv_buf;

  MessageHandler *handler;

  // We establish random remote ids since IP Adresses can be spoofed
  u32 remote_id;

  u32 sequence_id;
  u32 ack;
  u32 ack_bitfield;

  u32 message_id;
};

} // namespace Net
