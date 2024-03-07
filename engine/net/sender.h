#pragma once

#include "core/types.h"

#include "io/input_map.h"
#include "message_builder.h"
#include "message_handler.h"

#include <asio.hpp>

namespace Net {

class Sender {
public:
  Sender(std::shared_ptr<asio::ip::udp::socket> socket,
         asio::ip::udp::endpoint endpoint, u64 client_salt);
  Sender(asio::io_context &context, u32 port, asio::ip::udp::endpoint endpoint);
  Sender(asio::io_context &context);

  void write_connection_requested();
  void write_connection_accepted(u8 client_index);
  void write_connection_denied();
  void write_challenge();
  void write_challenge_response();
  void write_disconnected();
  void write_ping();
  void write_user_inputs(const InputMap &inputs);

  void write_disconnected_blocking();

  void bind(const asio::ip::udp::endpoint &endpoint, u64 client_salt);
  void update_salts(u64 client_salt, u64 server_salt);

  bool connected_to(const asio::ip::udp::endpoint &endpoint);
  bool matches_client_salt(u64 client_salt);
  bool matches_xor_salt(u64 xor_salt);
  bool matches_salts(u64 client_salt, u64 server_salt);

  /**
   * Updates the sender's acks based on the sequence id of the message
   * that has been received.
   * 
   * @return true iff the sequence id was updated (the message is not out of date)
   */
  bool update_acks(u32 sequence_id);

private:
  MessageBuilder message_scaffold(Net::MessageType type);

  void fill_buffer(const Message &message);

  void write_message(const Message &message);
  void write_message_blocking(const Message &message);

private:
  std::shared_ptr<asio::ip::udp::socket> socket;
  asio::ip::udp::endpoint send_endpoint;

  std::vector<u8> send_buf;

  u64 client_salt;
  u64 server_salt;

  u32 sequence_id;
  u32 ack;
  u32 ack_bitfield;

  u32 message_id;
};

} // namespace Net