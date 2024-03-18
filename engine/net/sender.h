#pragma once

#include "io/input_map.h"
#include "message_builder.h"
#include "message_handler.h"

#include <asio.hpp>

namespace Net {

class Sender {
public:
  Sender(std::shared_ptr<asio::ip::udp::socket> socket,
         asio::ip::udp::endpoint endpoint, uint64_t client_salt);
  Sender(asio::io_context &context, uint32_t port,
         asio::ip::udp::endpoint endpoint);
  Sender(asio::io_context &context);

  void write_connection_requested();
  void write_connection_accepted(uint8_t client_index);
  void write_connection_denied();
  void write_challenge();
  void write_challenge_response();
  void write_disconnected();
  void write_ping();
  void write_user_inputs(const InputMap &inputs);
  void write_world_state(const std::vector<uint8_t> &serialized_state);

  void write_disconnected_blocking();

  void bind(const asio::ip::udp::endpoint &endpoint, uint64_t client_salt);
  void update_salts(uint64_t client_salt, uint64_t server_salt);

  bool connected_to(const asio::ip::udp::endpoint &endpoint);
  bool matches_client_salt(uint64_t client_salt);
  bool matches_xor_salt(uint64_t xor_salt);
  bool matches_salts(uint64_t client_salt, uint64_t server_salt);

  /**
   * Updates the sender's acks based on the sequence id of the message
   * that has been received.
   *
   * @return true iff the sequence id was updated (the message is not out of
   * date)
   */
  bool update_acks(uint32_t sequence_id);

private:
  MessageBuilder message_scaffold(Net::MessageType type);

  void fill_buffer(const Message &message);

  void write_message(const Message &message);
  void write_message_blocking(const Message &message);

private:
  std::shared_ptr<asio::ip::udp::socket> socket;
  asio::ip::udp::endpoint send_endpoint;

  std::vector<uint8_t> send_buf;

  uint64_t client_salt;
  uint64_t server_salt;

  uint32_t sequence_id;
  uint32_t ack;
  uint32_t ack_bitfield;

  uint32_t message_id;
};

} // namespace Net
