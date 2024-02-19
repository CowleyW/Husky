#include "server.h"

#include "io/logging.h"
#include "net/connection.h"
#include "net/message_handler.h"

Net::Server::Server(u32 port, u8 max_clients)
    : port(port), max_clients(max_clients), num_connected_clients(0), clients(),
      context(std::make_unique<asio::io_context>()), listener(*context, port),
      recv_buf(1024), handler(nullptr) {
  for (u8 client = 0; client < max_clients; client += 1) {
    this->clients.emplace_back(Net::Connection(*this->context));
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

Result<Net::Connection *const>
Net::Server::get_client(const asio::ip::udp::endpoint &remote) {
  for (Connection &c : this->clients) {
    if (c.matches_remote(remote)) {
      return Result<Net::Connection *const>::ok(&c);
    }
  }

  return Result<Net::Connection *const>::err("No such connection");
}

bool Net::Server::has_open_client() {
  for (Connection &c : this->clients) {
    if (!c.is_connected()) {
      return true;
    }
  }

  return false;
}

void Net::Server::accept(const asio::ip::udp::endpoint &remote) {
  bool found_slot = false;

  for (Connection &c : this->clients) {
    if (!c.is_connected()) {
      c.bind(remote);
      c.write_connection_accepted();

      found_slot = true;
      break;
    }
  }

  if (!found_slot) {
    io::warn("Attempted to accept client when no slot was available.");
  }
}

void Net::Server::deny_connection(const asio::ip::udp::endpoint &remote) {
  Connection temp(*this->context);
  temp.bind(remote);

  temp.write_connection_denied();
}
