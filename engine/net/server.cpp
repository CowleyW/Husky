#include "server.h"

#include "io/logging.h"
#include "net/message_handler.h"
#include "util/serialize.h"
#include "core/world_state.h"

#include "core/random.h"

Net::Server::Server(u32 port, u8 max_clients)
    : port(port), max_clients(max_clients), num_connected_clients(0), clients(),
      context(std::make_unique<asio::io_context>()), listener(*context, port),
      recv_buf(1024), denier(*context), new_clients(), disconnected_clients() {
  for (u8 client = 0; client < max_clients; client += 1) {
    this->clients.emplace_back(Net::ClientSlot(*this->context, client));
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
Net::Server::get_by_endpoint(const asio::ip::udp::endpoint &remote) {
  for (ClientSlot &c : this->clients) {
    if (c.connected_to(remote)) {
      &c;
    }
  }

  return {};
}

std::optional<Net::ClientSlot *const>
Net::Server::get_by_client_salt(u64 client_salt,
                                const asio::ip::udp::endpoint &remote) {
  for (ClientSlot &c : this->clients) {
    if (c.matches_client_salt(client_salt) && c.connected_to(remote)) {
      return &c;
    }
  }

  return {};
}
std::optional<Net::ClientSlot *const>
Net::Server::get_by_xor_salt(u64 xor_salt,
                             const asio::ip::udp::endpoint &remote) {
  for (ClientSlot &c : this->clients) {
    if (c.matches_xor_salt(xor_salt) && c.connected_to(remote)) {
      return &c;
    }
  }

  return {};
}

std::optional<Net::ClientSlot *const>
Net::Server::get_by_xor_salt(u64 xor_salt) {
  for (ClientSlot &c : this->clients) {
    if (c.matches_xor_salt(xor_salt)) {
      return &c;
    }
  }

  return {};
}
std::optional<Net::ClientSlot *const>
Net::Server::get_by_salts(u64 client_salt, u64 server_salt,
                          const asio::ip::udp::endpoint &remote) {
  for (ClientSlot &c : this->clients) {
    if (c.matches_salts(client_salt, server_salt)) {
      return &c;
    }
  }

  return {};
}

std::optional<Net::ClientSlot *const> Net::Server::get_open() {
  for (ClientSlot &c : this->clients) {
    if (c.connection_status() == Net::ConnectionStatus::Disconnected) {
      return &c;
    }
  }

  return {};
}

std::vector<Net::ClientSlot> &Net::Server::get_clients() {
  return this->clients;
}

std::optional<u8> Net::Server::next_new_client() {
  if (this->new_clients.empty()) {
    return {};
  } else {
    u8 client_index = this->new_clients.front();
    this->new_clients.pop();
    return client_index;
  }
}

std::optional<u8> Net::Server::next_disconnected_client() {
  if (this->disconnected_clients.empty()) {
    return {};
  } else {
    u8 client_index = this->disconnected_clients.front();
    this->disconnected_clients.pop();
    return client_index;
  }
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

void Net::Server::send_world_state(const WorldState &world_state) {
  io::debug("{} clients in world state", world_state.player_count());
  std::vector<u8> buf(world_state.packed_size());
  world_state.serialize_into(buf, 0);

  for (ClientSlot &c : this->clients) {
    c.send_world_state(buf);
  }
}

// Can we get rid of the Server::accept() method? It doesn't seem to get called
// anywhere
void Net::Server::accept(const asio::ip::udp::endpoint &remote,
                         u64 client_salt) {
  bool found_slot = false;

  for (ClientSlot &c : this->clients) {
    if (!c.is_connected()) {
      c.bind(remote, client_salt);
      c.accept();

      found_slot = true;
      break;
    }
  }

  if (!found_slot) {
    io::warn("Attempted to accept client when no slot was available.");
  }
}

void Net::Server::challenge(const asio::ip::udp::endpoint &remote,
                            u64 client_salt) {
  auto maybe_client = this->get_by_client_salt(client_salt, remote);

  if (maybe_client.has_value()) {
    maybe_client.value()->send_challenge();
    return;
  }

  auto maybe_open = this->get_open();

  if (maybe_open.has_value()) {
    auto client = maybe_open.value();
    client->bind(remote, client_salt);
    client->send_challenge();
  } else {
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
  io::debug("Received ConnectionRequested with salt {}", message.header.salt);

  std::optional<Net::ClientSlot *const> result =
      this->get_by_client_salt(message.header.salt, remote);
  if (result.has_value()) {
    io::debug("Client already connected.");
    Net::ClientSlot *const connection = result.value();
    connection->send_challenge();
  } else if (this->has_open_slot()) {
    this->challenge(remote, message.header.salt);
  } else {
    this->deny_connection(remote);
  }
}

void Net::Server::on_challenge_response(const Net::Message &message,
                                        const asio::ip::udp::endpoint &remote) {
  if (message.body.size() != 8 + Net::Message::CHALLENGE_RESPONSE_PADDING) {
    io::error("Invalid ChallengeResponse padding: {} (actual) != {} (expected)",
              message.body.size(),
              8 + Net::Message::CHALLENGE_RESPONSE_PADDING);
    return;
  }

  io::debug("Received ChallengeResponse with salt {}", message.header.salt);
  u64 server_salt = Serialize::deserialize_u64(MutBuf<u8>(message.body));
  u64 client_salt = message.header.salt ^ server_salt;

  auto maybe_client = this->get_by_salts(client_salt, server_salt, remote);
  if (maybe_client.has_value()) {
    io::debug("Client passed challenge");
    maybe_client.value()->accept();
    this->new_clients.push(maybe_client.value()->index());
  } else {
    io::debug("Client failed challenge");
  }
}

void Net::Server::on_disconnected(const Net::Message &message,
                                  const asio::ip::udp::endpoint &remote) {
  auto maybe = this->get_by_xor_salt(message.header.salt, remote);

  if (maybe.has_value()) {
    auto client = maybe.value();

    client->disconnect();
    this->disconnected_clients.push(client->index());
  } else {
    io::warn("Received message from unknown remote id {}.",
             message.header.salt);
  }
}

void Net::Server::on_ping(const Net::Message &message,
                          const asio::ip::udp::endpoint &remote) {
  auto maybe = this->get_by_xor_salt(message.header.salt, remote);

  if (maybe.has_value()) {
    auto client = maybe.value();
    client->add_message(message);
  } else {
    io::warn("Received ping from unknown client {}.", message.header.salt);
  }
}

void Net::Server::on_user_inputs(const Net::Message &message,
                                 const asio::ip::udp::endpoint &remote) {
  std::optional<Net::ClientSlot *const> maybe =
      this->get_by_xor_salt(message.header.salt, remote);

  if (maybe.has_value()) {
    auto client = maybe.value();
    client->add_message(message);
  } else {
    io::warn("Received inputs from unknown client {}.", message.header.salt);
  }
}