#include "sender.h"

#include "io/input_map.h"

#include <asio.hpp>
#include <core/def.h>
#include <crypto/checksum.h>
#include <io/logging.h>
#include <util/serialize.h>

Net::Sender::Sender(std::shared_ptr<asio::ip::udp::socket> socket,
                    asio::ip::udp::endpoint endpoint, u32 remote_id)
    : socket(socket), send_endpoint(endpoint), send_buf(0),
      remote_id(remote_id), ack(0), ack_bitfield(0), sequence_id(0),
      message_id(0) {}

Net::Sender::Sender(asio::io_context &context, u32 port,
                    asio::ip::udp::endpoint endpoint)
    : socket(std::make_shared<asio::ip::udp::socket>(
          context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))),
      send_endpoint(endpoint), send_buf(0), remote_id(0), ack(0),
      ack_bitfield(0), sequence_id(0), message_id(0) {}

Net::Sender::Sender(asio::io_context &context)
    : socket(std::make_shared<asio::ip::udp::socket>(context)), send_buf(0),
      remote_id(0), ack(0), ack_bitfield(0), sequence_id(0),
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

void Net::Sender::write_connection_accepted() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionAccepted).build();

  this->write_message(message);
}

void Net::Sender::write_connection_denied() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionDenied).build();

  this->write_message(message);
}

void Net::Sender::write_ping() {
  Net::Message message = this->message_scaffold(Net::MessageType::Ping).build();

  this->write_message(message);
}

void Net::Sender::write_user_inputs(const InputMap &inputs) {
  std::vector<u8> input_map(InputMap::packed_size());
  inputs.serialize_into(input_map, 0);

  Net::Message message = this->message_scaffold(Net::MessageType::UserInputs)
                             .with_body(input_map)
                             .build();

  this->write_message(message);
}

void Net::Sender::bind(const asio::ip::udp::endpoint &endpoint, u32 remote_id) {
  this->send_endpoint = endpoint;

  this->remote_id = remote_id;

  this->ack = 0;
  this->ack_bitfield = 0;

  this->message_id = 0;
  this->sequence_id = 0;
}

bool Net::Sender::connected_to(const asio::ip::udp::endpoint &endpoint) {
  return this->send_endpoint == endpoint;
}

Net::MessageBuilder Net::Sender::message_scaffold(Net::MessageType type) {
  return Net::MessageBuilder(type)
      .with_ids(this->remote_id, this->sequence_id, this->message_id)
      .with_acks(this->ack, this->ack_bitfield)
      .with_padding(0);
}

void Net::Sender::write_message(const Net::Message &message) {
  u32 message_size = message.packed_size() + Net::PacketHeader::packed_size();
  this->send_buf.resize(message_size);
  message.serialize_into(this->send_buf, PacketHeader::packed_size());

  // Serialize the protocol ID
  u32 offset = Serialize::serialize_u32(NET_PROTOCOL_ID, this->send_buf, 0);

  // Calculate and serialize the checksum
  u32 crc = Crypto::calculate_checksum(
      &this->send_buf[PacketHeader::packed_size()], message.packed_size());
  Serialize::serialize_u32(crc, this->send_buf, offset);

  auto on_send = [this](const asio::error_code &err, u64 size) {
    if (err) {
      io::error(err.message());
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