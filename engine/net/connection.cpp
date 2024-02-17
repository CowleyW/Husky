#include "connection.h"

#include "core/def.h"
#include "crypto/checksum.h"
#include "io/logging.h"
#include "net/message.h"
#include "net/message_builder.h"
#include "util/err.h"
#include "util/serialize.h"

#include <memory>
#include <random>
#include <vector>

Net::Connection::Connection(asio::io_context &context)
    : connected(false), remote_id(0), socket(context), send_buf() {
  socket.open(asio::ip::udp::v4());
}

void Net::Connection::free() {
  this->connected = false;
  this->remote_id = 0;
}

Err Net::Connection::bind(const asio::ip::udp::endpoint &remote) {
  if (this->connected) {
    return Err::err("Client is already connected to this slot");
  }

  this->connected = true;

  // Create random remote_id -- probably shouldn't happen here. The server
  // should be in control of ids and should supply them to the clients
  std::random_device device;
  std::mt19937 rng(device());
  std::uniform_int_distribution<u32> distrib;
  this->remote_id = distrib(rng);
  io::debug("New remote id {}", this->remote_id);

  this->remote = remote;

  return Err::ok();
}

bool Net::Connection::is_connected() { return this->connected; }

bool Net::Connection::matches_id(u32 id) { return this->remote_id == id; }

bool Net::Connection::matches_remote(const asio::ip::udp::endpoint &remote) {
  return this->remote == remote;
}

void Net::Connection::write_message(const Net::Message &message) {
  u32 message_size = message.packed_size() + Net::PacketHeader::packed_size();
  if (this->send_buf.size() < message_size) {
    this->send_buf.resize(message_size);
  }
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

    io::debug("Sent {} bytes to {}.", size, this->remote.address().to_string());
  };
  this->socket.async_send_to(asio::buffer(this->send_buf), this->remote,
                             on_send);
}

void Net::Connection::write_connection_requested() {
  Net::MessageBuilder builder(Net::MessageType::ConnectionRequested);
  Net::Message message =
      builder.with_ids(this->remote_id, this->sequence_id, this->message_id)
          .with_acks(this->ack, this->ack_bitfield)
          .with_padding(512)
          .build();

  this->write_message(message);
}

void Net::Connection::write_connection_accepted() {
  Net::MessageBuilder builder(Net::MessageType::ConnectionAccepted);
  Net::Message message =
      builder.with_ids(this->remote_id, this->sequence_id, this->message_id)
          .with_acks(this->ack, this->ack_bitfield)
          .build();

  this->write_message(message);
}

void Net::Connection::write_connection_denied() {
  Net::MessageBuilder builder(Net::MessageType::ConnectionDenied);
  Net::Message message =
      builder.with_ids(this->remote_id, this->sequence_id, this->message_id)
          .with_acks(this->ack, this->ack_bitfield)
          .build();

  this->write_message(message);
}
