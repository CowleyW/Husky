#include "client_slot.h"

#include <asio.hpp>

Net::ClientSlot::ClientSlot(asio::io_context &context)
    : sender(std::make_unique<Net::Sender>(context)), connected(false) {}

void Net::ClientSlot::bind(const asio::ip::udp::endpoint &endpoint,
                           u32 remote_id) {
  this->sender->bind(endpoint, remote_id);
  this->connected = true;
}

bool Net::ClientSlot::is_connected() { return this->connected; }

bool Net::ClientSlot::connected_to(const asio::ip::udp::endpoint &endpoint) {
  return this->sender->connected_to(endpoint);
}

Net::Sender &Net::ClientSlot::get_sender() { return *this->sender; }