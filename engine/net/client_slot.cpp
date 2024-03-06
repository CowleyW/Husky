#include "client_slot.h"

#include <asio.hpp>

Net::ClientSlot::ClientSlot(asio::io_context &context)
    : sender(std::make_unique<Net::Sender>(context)), connected(false),
      message_queue() {}

void Net::ClientSlot::bind(const asio::ip::udp::endpoint &endpoint,
                           u32 remote_id) {
  this->sender->bind(endpoint, remote_id);
  this->connected = true;
}

bool Net::ClientSlot::is_connected() { return this->connected; }

bool Net::ClientSlot::connected_to(const asio::ip::udp::endpoint &endpoint) {
  return this->sender->connected_to(endpoint);
}

bool Net::ClientSlot::matches_id(u32 remote_id) {
  return this->sender->matches_id(remote_id);
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
  }
}

void Net::ClientSlot::ping() {
  if (this->connected) {
    this->sender->write_ping();
  }
}