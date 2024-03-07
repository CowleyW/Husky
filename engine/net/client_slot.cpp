#include "client_slot.h"

#include <asio.hpp>

Net::ClientSlot::ClientSlot(asio::io_context &context)
    : sender(std::make_unique<Net::Sender>(context)),
      status(Net::ConnectionStatus::Disconnected), message_queue(),
      last_message(std::chrono::steady_clock::now()) {}

void Net::ClientSlot::bind(const asio::ip::udp::endpoint &endpoint,
                           u32 remote_id) {
  this->sender->bind(endpoint, remote_id);
  this->status = Net::ConnectionStatus::Connected;

  this->last_message = std::chrono::steady_clock::now();
}

bool Net::ClientSlot::is_connected() {
  return this->status == Net::ConnectionStatus::Connected;
}

bool Net::ClientSlot::connected_to(const asio::ip::udp::endpoint &endpoint) {
  return this->sender->connected_to(endpoint);
}

bool Net::ClientSlot::matches_id(u32 remote_id) {
  return this->sender->matches_id(remote_id);
}

Net::ConnectionStatus Net::ClientSlot::connection_status() {
  return this->status;
}

Net::Sender &Net::ClientSlot::get_sender() { return *this->sender; }

u32 Net::ClientSlot::remote_id() { return this->sender->get_remote_id(); }

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