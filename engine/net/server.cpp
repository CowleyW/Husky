#include "server.h"

#include "io/logging.h"
#include "net/message_handler.h"

Net::Server::Server(u32 port, u8 max_clients)
    : port(port), max_clients(max_clients), num_connected_clients(0), clients(),
      context(std::make_unique<asio::io_context>()), listener(*context, port),
      recv_buf(1024), handler(nullptr), denier(*context) {
  for (u8 client = 0; client < max_clients; client += 1) {
    this->clients.emplace_back(Net::ClientSlot(*this->context));
  }
}

void Net::Server::begin() {
  io::debug("Beginning server.");

  this->context_thread = std::thread([this]() { this->context->run(); });

  this->listener.listen();
}

void Net::Server::register_callbacks(Net::MessageHandler *handler) {
  this->listener.register_callbacks(handler);
}

std::optional<Net::ClientSlot *const>
Net::Server::get_client(const asio::ip::udp::endpoint &remote) {
  for (ClientSlot &c : this->clients) {
    if (c.connected_to(remote)) {
      &c;
    }
  }

  return {};
}

bool Net::Server::has_open_slot() {
  for (ClientSlot &c : this->clients) {
    if (!c.is_connected()) {
      return true;
    }
  }

  return false;
}

void Net::Server::accept(const asio::ip::udp::endpoint &remote) {
  bool found_slot = false;

  for (ClientSlot &c : this->clients) {
    if (!c.is_connected()) {
      // Some meaningless remote id for now
      u32 remote_id = 0x102030;
      c.bind(remote, remote_id);
      c.get_sender().write_connection_accepted();

      found_slot = true;
      break;
    }
  }

  if (!found_slot) {
    io::warn("Attempted to accept client when no slot was available.");
  }
}

void Net::Server::deny_connection(const asio::ip::udp::endpoint &remote) {
  denier.bind(remote, 0);

  denier.write_connection_denied();
}
