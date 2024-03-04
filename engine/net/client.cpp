#include "client.h"
#include "fmt/format.h"
#include "io/logging.h"
#include "net/message_handler.h"

#include <asio.hpp>

#include <memory>

Net::Client::Client(u32 server_port, u32 client_port)
    : context(std::make_unique<asio::io_context>()),
      handler(nullptr) {
  asio::ip::udp::resolver resolver(*this->context);
  asio::ip::udp::endpoint server_endpoint = *resolver
                               .resolve(asio::ip::udp::v4(), "127.0.0.1",
                                        fmt::format("{}", server_port))
                               .begin();
  auto socket = std::make_shared<asio::ip::udp::socket>(
      *context, asio::ip::udp::endpoint(asio::ip::udp::v4(), client_port));
  this->listener = std::make_unique<Net::Listener>(socket);
  this->sender = std::make_unique<Net::Sender>(socket, server_endpoint, 0);
}

void Net::Client::begin() {
  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->listener->listen();
  this->sender->write_connection_requested();
}

void Net::Client::register_callbacks(Net::MessageHandler *handler) {
  this->listener->register_callbacks(handler);
}

void Net::Client::shutdown() {
  this->context->stop();
  this->context_thread.join();
}

void Net::Client::ping_server() { this->sender->write_ping(); }

void Net::Client::send_inputs(const InputMap &inputs) {
  this->sender->write_user_inputs(inputs);
}
