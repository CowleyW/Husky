#include "client_slot.h"

#include <asio.hpp>

Net::ClientSlot::ClientSlot(asio::io_context &context, u8 client_index)
    : sender(std::make_unique<Net::Sender>(context)),
      status(Net::ConnectionStatus::Disconnected), message_queue(),
      last_message(std::chrono::steady_clock::now()),
      client_index(client_index) {}

void Net::ClientSlot::bind(const asio::ip::udp::endpoint &endpoint, u64 salt) {
  this->sender->bind(endpoint, salt);
  this->status = Net::ConnectionStatus::Connecting;

  this->last_message = std::chrono::steady_clock::now();
}

bool Net::ClientSlot::is_connected() {
  return this->status == Net::ConnectionStatus::Connected;
}

bool Net::ClientSlot::connected_to(const asio::ip::udp::endpoint &endpoint) {
  return this->sender->connected_to(endpoint);
}

bool Net::ClientSlot::matches_xor_salt(u64 xor_salt) {
  return this->sender->matches_xor_salt(xor_salt);
}

bool Net::ClientSlot::matches_client_salt(u64 client_salt) {
  return this->sender->matches_client_salt(client_salt);
}

bool Net::ClientSlot::matches_salts(u64 client_salt, u64 server_salt) {
  return this->sender->matches_salts(client_salt, server_salt);
}

Net::ConnectionStatus Net::ClientSlot::connection_status() {
  return this->status;
}

u8 Net::ClientSlot::index() { return this->client_index; }

std::optional<Net::Message> Net::ClientSlot::next_message() {
  if (this->message_queue.empty()) {
    return {};
  } else {
    Net::Message ret = this->message_queue.front();
    this->message_queue.pop();
    return ret;
  }
}

void Net::ClientSlot::add_message(const Net::Message &message) {
  if (this->sender->update_acks(message.header.sequence_id)) {
    this->message_queue.push(message);

    this->last_message = std::chrono::steady_clock::now();
  }
}

void Net::ClientSlot::accept() {
  this->status = Net::ConnectionStatus::Connected;

  this->sender->write_connection_accepted(this->client_index);
}

void Net::ClientSlot::send_challenge() { this->sender->write_challenge(); }

void Net::ClientSlot::ping() {
  if (this->is_connected()) {
    this->sender->write_ping();
  }
}

void Net::ClientSlot::disconnect() {
  if (this->status != Net::ConnectionStatus::Disconnected) {
    this->sender->write_disconnected();
    this->status = Net::ConnectionStatus::Disconnected;
  }
}

bool Net::ClientSlot::maybe_timeout() {
  auto now = std::chrono::steady_clock::now();

  if ((now - this->last_message) > this->timeout_wait) {
    this->disconnect();
    return true;
  } else {
    return false;
  }
}