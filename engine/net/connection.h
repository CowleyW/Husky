#pragma once

#include "asio/io_context.hpp"
#include "message.h"

#include "core/types.h"
#include "util/err.h"

#include <asio.hpp>

namespace Net {

class Connection {
public:
  Connection(asio::io_context &context);

  void free();
  Err bind(const asio::ip::udp::endpoint &remote);

  bool is_connected();
  bool matches_id(u32 id);
  bool matches_remote(const asio::ip::udp::endpoint &remote);
  void write_message(const Message &message);

  void write_connection_requested();
  void write_connection_accepted();
  void write_connection_denied();

private:
  bool connected;

  // The remote (client) endpoint that we write to
  asio::ip::udp::endpoint remote;
  asio::ip::udp::socket socket;

  std::vector<u8> send_buf;

  // We establish random remote ids since IP Adresses can be spoofed
  u32 remote_id;

  u32 sequence_id;
  u32 ack;
  u32 ack_bitfield;

  u32 message_id;
};

} // namespace Net
