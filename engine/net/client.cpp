#include "client.h"
#include "core/random.h"
#include "fmt/format.h"
#include "io/logging.h"
#include "util/serialize.h"

#include <asio.hpp>

#include <memory>

Net::Client::Client(uint32_t server_port, uint32_t client_port)
    : client_salt(Random().random_u64()),
      server_salt(0),
      status(Net::ConnectionStatus::Disconnected),
      context(std::make_unique<asio::io_context>()) {
  io::debug("rolled salt {}", this->client_salt);
  using udp = asio::ip::udp;
  udp::resolver resolver(*this->context);
  udp::endpoint server_endpoint =
      *resolver.resolve(udp::v4(), "127.0.0.1", fmt::format("{}", server_port))
           .begin();
  auto socket = std::make_shared<udp::socket>(
      *context,
      udp::endpoint(udp::v4(), client_port));
  this->listener = std::make_unique<Net::Listener>(socket);
  this->sender =
      std::make_unique<Net::Sender>(socket, server_endpoint, this->client_salt);
}

void Net::Client::begin() {
  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->listener->register_callbacks(this);
  this->listener->listen();

  this->sender->write_connection_requested();
  this->status = Net::ConnectionStatus::Connecting;
}

void Net::Client::shutdown() {
  this->sender->write_disconnected_blocking();
  this->context->stop();
  this->context_thread.join();
}

bool Net::Client::maybe_timeout() {
  auto now = std::chrono::steady_clock::now();

  if ((now - this->last_message) > this->timeout_wait) {
    this->disconnect();
    return true;
  } else {
    return false;
  }
}

void Net::Client::ping_server() {
  this->sender->write_ping();
}

void Net::Client::send_inputs(const InputMap &inputs) {
  this->sender->write_user_inputs(inputs);
}

void Net::Client::disconnect() {
  if (this->status != Net::ConnectionStatus::Disconnected) {
    this->sender->write_disconnected();
    this->status = Net::ConnectionStatus::Disconnected;
  }
}

bool Net::Client::is_connected() {
  return this->status == Net::ConnectionStatus::Connected;
}

Net::ConnectionStatus Net::Client::connection_status() {
  return this->status;
}

std::optional<Net::Message> Net::Client::next_message() {
  if (!this->messages.empty()) {
    Net::Message ret = this->messages.front();
    this->messages.pop();

    return ret;
  } else {
    return {};
  }
}

void Net::Client::on_connection_accepted(
    const Net::Message &message,
    const asio::ip::udp::endpoint &remote) {
  if (message.body.size() != Net::Message::CONNECTION_ACCEPTED_BODY_SIZE) {
    io::error(
        "Invalid ConnectionAccepted size: {} (actual) != {} (expected)",
        message.body.size(),
        Net::Message::CONNECTION_ACCEPTED_BODY_SIZE);
    return;
  }

  if (!this->is_connected()) {
    this->status = Net::ConnectionStatus::Connected;
  }

  this->add_message(message);
  MutBuf<uint8_t> mutbuf(message.body);
  io::debug("connection accepted body: {}", Serialize::deserialize_u8(mutbuf));
  io::debug("connectino accepted salt: {}", message.header.salt);
}

void Net::Client::on_connection_denied(
    const Net::Message &message,
    const asio::ip::udp::endpoint &remote) {
  this->add_message(message);
}

void Net::Client::on_challenge(
    const Net::Message &message,
    const asio::ip::udp::endpoint &remote) {
  if (message.body.size() != 8) {
    io::error(
        "Invalid Challenge size: {} (actual) != 8 (expected)",
        message.body.size());
  }

  MutBuf<uint8_t> mutbuf(message.body);
  this->server_salt = Serialize::deserialize_u64(mutbuf);
  io::info(
      "Server salt: {}, xor_salt: {}",
      this->server_salt,
      message.header.salt);

  this->sender->update_salts(this->client_salt, this->server_salt);
  this->sender->write_challenge_response();
}

void Net::Client::on_ping(
    const Net::Message &message,
    const asio::ip::udp::endpoint &remote) {
  this->add_message(message);
}

void Net::Client::on_world_snapshot(
    const Net::Message &message,
    const asio::ip::udp::endpoint &remote) {
  this->add_message(message);
}

void Net::Client::add_message(const Net::Message &message) {
  if (this->sender->update_acks(message.header.sequence_id)) {
    this->messages.push(message);

    this->last_message = std::chrono::steady_clock::now();
  }
}
