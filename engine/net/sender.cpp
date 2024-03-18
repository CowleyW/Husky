#include "sender.h"

#include "core/def.h"
#include "core/random.h"
#include "crypto/checksum.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "util/serialize.h"

#include <asio.hpp>

Net::Sender::Sender(std::shared_ptr<asio::ip::udp::socket> socket,
                    asio::ip::udp::endpoint endpoint, uint64_t client_salt)
    : socket(socket), send_endpoint(endpoint), send_buf(0), client_salt(0),
      server_salt(0), sequence_id(0), ack(0), ack_bitfield(0), message_id(0) {}

Net::Sender::Sender(asio::io_context &context, uint32_t port,
                    asio::ip::udp::endpoint endpoint)
    : socket(std::make_shared<asio::ip::udp::socket>(
          context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))),
      send_endpoint(endpoint), send_buf(0), client_salt(0), server_salt(0),
      sequence_id(0), ack(0), ack_bitfield(0), message_id(0) {}

Net::Sender::Sender(asio::io_context &context)
    : socket(std::make_shared<asio::ip::udp::socket>(context)), send_buf(0),
      client_salt(0), server_salt(0), sequence_id(0), ack(0), ack_bitfield(0),
      message_id(0) {
  this->socket->open(asio::ip::udp::v4());
}

void Net::Sender::write_connection_requested() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionRequested)
          .with_padding(Net::Message::CONNECTION_REQUESTED_PADDING)
          .build();

  this->write_message(message);
}

void Net::Sender::write_connection_accepted(uint8_t client_index) {
  std::vector<uint8_t> body(sizeof(client_index));
  Serialize::serialize_u8(client_index, body, 0);
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionAccepted)
          .with_body(body)
          .build();

  this->write_message(message);
}

void Net::Sender::write_connection_denied() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionDenied).build();

  this->write_message(message);
}

void Net::Sender::write_challenge() {
  std::vector<uint8_t> body(sizeof(this->server_salt));
  Serialize::serialize_u64(this->server_salt, body, 0);
  Net::Message message = this->message_scaffold(Net::MessageType::Challenge)
                             .with_body(body)
                             .build();

  this->write_message(message);
}

void Net::Sender::write_challenge_response() {
  std::vector<uint8_t> body(sizeof(this->server_salt));
  Serialize::serialize_u64(this->server_salt, body, 0);

  Net::Message message =
      this->message_scaffold(Net::MessageType::ChallengeResponse)
          .with_body(body)
          .with_padding(512)
          .build();

  this->write_message(message);
}

void Net::Sender::write_disconnected() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::Disconnected).build();

  this->write_message(message);
}

void Net::Sender::write_ping() {
  Net::Message message = this->message_scaffold(Net::MessageType::Ping).build();

  this->write_message(message);
}

void Net::Sender::write_user_inputs(const InputMap &inputs) {
  std::vector<uint8_t> input_map(InputMap::packed_size());
  inputs.serialize_into(input_map, 0);

  Net::Message message = this->message_scaffold(Net::MessageType::UserInputs)
                             .with_body(input_map)
                             .build();

  this->write_message(message);
}

void Net::Sender::write_disconnected_blocking() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::Disconnected).build();

  this->write_message_blocking(message);
}

void Net::Sender::write_world_state(
    const std::vector<uint8_t> &serialized_state) {
  Net::Message message = this->message_scaffold(Net::MessageType::WorldSnapshot)
                             .with_body(serialized_state)
                             .build();

  this->write_message(message);
}

void Net::Sender::bind(const asio::ip::udp::endpoint &endpoint,
                       uint64_t client_salt) {
  this->send_endpoint = endpoint;

  this->client_salt = client_salt;
  this->server_salt = Random().random_u64();
  io::debug("Rolled new server salt {}", this->server_salt);

  this->ack = 0;
  this->ack_bitfield = 0;

  this->message_id = 0;
  this->sequence_id = 0;
}

void Net::Sender::update_salts(uint64_t client_salt, uint64_t server_salt) {
  this->client_salt = client_salt;
  this->server_salt = server_salt;
}

bool Net::Sender::connected_to(const asio::ip::udp::endpoint &endpoint) {
  return this->send_endpoint == endpoint;
}

bool Net::Sender::matches_client_salt(uint64_t client_salt) {
  return this->client_salt == client_salt;
}

bool Net::Sender::matches_xor_salt(uint64_t xor_salt) {
  return xor_salt == (this->client_salt ^ this->server_salt);
}

bool Net::Sender::matches_salts(uint64_t client_salt, uint64_t server_salt) {
  return this->client_salt == client_salt && this->server_salt == server_salt;
}

bool Net::Sender::update_acks(uint32_t sequence_id) {
  if (this->ack < sequence_id) {
    uint32_t diff = sequence_id - this->ack;

    this->ack = sequence_id;
    this->ack_bitfield = (this->ack_bitfield << diff) + 1;

    return true;
  } else {
    return false;
  }
}

Net::MessageBuilder Net::Sender::message_scaffold(Net::MessageType type) {
  return Net::MessageBuilder(type)
      .with_ids(this->sequence_id, this->message_id)
      .with_acks(this->ack, this->ack_bitfield)
      .with_padding(0)
      .with_salt(this->client_salt ^ this->server_salt);
}

void Net::Sender::fill_buffer(const Net::Message &message) {
  uint32_t message_size =
      message.packed_size() + Net::PacketHeader::packed_size();
  this->send_buf.resize(message_size);
  message.serialize_into(this->send_buf, PacketHeader::packed_size());

  // Serialize the protocol ID
  uint32_t offset =
      Serialize::serialize_u32(NET_PROTOCOL_ID, this->send_buf, 0);

  // Calculate and serialize the checksum
  uint32_t crc = Crypto::calculate_checksum(
      &this->send_buf[PacketHeader::packed_size()], message.packed_size());
  Serialize::serialize_u32(crc, this->send_buf, offset);
}

void Net::Sender::write_message(const Net::Message &message) {
  this->fill_buffer(message);

  auto on_send = [this](const asio::error_code &err, uint64_t size) {
    if (err) {
      io::error("Sender::write_message -- {}", err.message());
      return;
    }

    io::debug("Sent {} bytes to {}:{}.", size,
              this->send_endpoint.address().to_string(),
              this->send_endpoint.port());
  };
  this->socket->async_send_to(asio::buffer(this->send_buf), this->send_endpoint,
                              on_send);

  this->sequence_id += 1;
  this->message_id += 1;
}

void Net::Sender::write_message_blocking(const Net::Message &message) {
  this->fill_buffer(message);

  this->socket->send_to(asio::buffer(this->send_buf), this->send_endpoint);
}
