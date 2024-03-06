#include "client.h"
#include "fmt/format.h"
#include "io/logging.h"
#include "message_handler.h"
#include "util/serialize.h"

#include <asio.hpp>

#include <memory>

Net::Client::Client(u32 server_port, u32 client_port)
    : context(std::make_unique<asio::io_context>()), connected(false) {
  using udp = asio::ip::udp;
  udp::resolver resolver(*this->context);
  udp::endpoint server_endpoint =
      *resolver.resolve(udp::v4(), "127.0.0.1", fmt::format("{}", server_port))
           .begin();
  auto socket = std::make_shared<udp::socket>(
      *context, udp::endpoint(udp::v4(), client_port));
  this->listener = std::make_unique<Net::Listener>(socket);
  this->sender = std::make_unique<Net::Sender>(socket, server_endpoint, 0);
}

void Net::Client::begin() {
  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->listener->register_callbacks(this);
  this->listener->listen();

  this->sender->write_connection_requested();
}

void Net::Client::shutdown() {
  this->context->stop();
  this->context_thread.join();
}

void Net::Client::ping_server() { this->sender->write_ping(); }

void Net::Client::send_inputs(const InputMap &inputs) {
  this->sender->write_user_inputs(inputs);
}

bool Net::Client::is_connected() { return this->connected; }

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
    const Net::Message &message, const asio::ip::udp::endpoint &remote) {
  if (message.body.size() != Net::Message::CONNECTION_ACCEPTED_BODY_SIZE) {
    io::error(
        "Invalid ConnectionAccepted size: {} (actual) != {} (expected)",
        message.body.size(), Net::Message::CONNECTION_ACCEPTED_BODY_SIZE);
    return;
  }

  if (!this->connected) {
    this->connected = true;

    u32 remote_id = Serialize::deserialize_u32(MutBuf<u8>(message.body));
    this->sender->set_remote_id(remote_id);
  }

  this->messages.push(message);
}

void Net::Client::on_connection_denied(const Net::Message &message,
                                       const asio::ip::udp::endpoint &remote) {
  this->messages.push(message);
}

void Net::Client::on_ping(const Net::Message &message,
                          const asio::ip::udp::endpoint &remote) {
  this->messages.push(message);
}
