#include "server.h"

#include "io/logging.h"
#include "net/message_handler.h"

#include <cstdlib>

Net::Server::Server(u32 port, u8 max_clients)
    : port(port), max_clients(max_clients), num_connected_clients(0), clients(),
      context(std::make_unique<asio::io_context>()), listener(*context, port),
      recv_buf(1024), handler(nullptr), denier(*context) {
  for (u8 client = 0; client < max_clients; client += 1) {
    this->clients.emplace_back(Net::ClientSlot(*this->context));
  }

  this->listener.register_callbacks(this);
}

void Net::Server::begin() {
  io::debug("Beginning server.");

  this->context_thread = std::thread([this]() { this->context->run(); });

  this->listener.listen();
}

void Net::Server::shutdown() {
  this->context->stop();
  this->context_thread.join();
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

std::optional<Net::ClientSlot *const>
Net::Server::get_client(u32 remote_id) {
  for (ClientSlot &c : this->clients) {
    if (c.matches_id(remote_id)) {
      return &c;
    }
  }

  return {};
}

std::optional<Net::ClientSlot *const>
Net::Server::get_client(u32 remote_id, const asio::ip::udp::endpoint &remote) {
  for (ClientSlot &c : this->clients) {
    if (c.matches_id(remote_id) && c.connected_to(remote)) {
      return &c;
    }
  }

  return {};
}

std::vector<Net::ClientSlot> &Net::Server::get_clients() {
  return this->clients;
}

bool Net::Server::has_open_slot() {
  for (ClientSlot &c : this->clients) {
    if (!c.is_connected()) {
      return true;
    }
  }

  return false;
}

void Net::Server::ping_all() {
  for (ClientSlot &c : this->clients) {
    c.ping();
  }
}

void Net::Server::disconnect(u32 remote_id) {
  auto maybe = this->get_client(remote_id);

  if (maybe.has_value()) {
    auto client = maybe.value();

    client->disconnect();
  }
}

void Net::Server::accept(const asio::ip::udp::endpoint &remote) {
  bool found_slot = false;

  for (ClientSlot &c : this->clients) {
    if (!c.is_connected()) {
      std::srand(std::time(nullptr));

      u32 remote_id = std::rand();
      io::debug("New client with id {}.", remote_id);

      c.bind(remote, remote_id);
      c.get_sender().write_connection_accepted(remote_id);

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

void Net::Server::on_connection_requested(
    const Net::Message &message, const asio::ip::udp::endpoint &remote) {
  if (message.body.size() != Net::Message::CONNECTION_REQUESTED_PADDING) {
    io::error(
        "Invalid ConnectionRequested padding: {} (actual) != {} (expected)",
        message.body.size(), Net::Message::CONNECTION_REQUESTED_PADDING);
    return;
  }
  io::debug("Received ConnectionRequested");

  std::optional<Net::ClientSlot *const> result = this->get_client(remote);
  if (result.has_value()) {
    io::debug("Client already connected.");
    Net::ClientSlot *const connection = result.value();
    connection->get_sender().write_connection_accepted(connection->remote_id());
  } else if (this->has_open_slot()) {
    this->accept(remote);
  } else {
    this->deny_connection(remote);
  }
}

void Net::Server::on_disconnected(const Net::Message &message,
                                  const asio::ip::udp::endpoint &remote) {
  auto maybe = this->get_client(message.header.remote_id, remote);

  if (maybe.has_value()) {
    auto client = maybe.value();
    client->add_message(message);
  } else {
    io::warn("Received message from unknown remote id {}.",
             message.header.remote_id);
  }
}

void Net::Server::on_ping(const Net::Message &message,
                          const asio::ip::udp::endpoint &remote) {
  auto maybe = this->get_client(message.header.remote_id, remote);

  if (maybe.has_value()) {
    auto client = maybe.value();
    client->add_message(message);
  } else {
    io::warn("Received message from unknown remote id {}.",
             message.header.remote_id);
  }
}

void Net::Server::on_user_inputs(const Net::Message &message,
                                 const asio::ip::udp::endpoint &remote) {
  std::optional<Net::ClientSlot *const> maybe =
      this->get_client(message.header.remote_id, remote);

  if (maybe.has_value()) {
    auto client = maybe.value();
    client->add_message(message);
  } else {
    io::warn("Received message from unknown remote id {}.",
             message.header.remote_id);
  }
}