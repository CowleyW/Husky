#include "connection.h"

#include "io/logging.h"
#include "net/message.h"
#include "net/message_builder.h"
#include "util/err.h"
#include <random>
#include <vector>

Net::Connection::Connection() : connected(false), remote_id(0) {}

void Net::Connection::free() {
  this->connected = false;
  this->remote_id = 0;
}

Err Net::Connection::bind(const asio::ip::udp::endpoint &remote) {
  if (this->connected) {
    return Err::err("Client is already connected to this slot");
  }

  this->connected = true;

  // Create random  remote_id
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
  io::debug("TODO: Write message to {}", this->remote_id);
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
