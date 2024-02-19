#include "client.h"
#include "asio/io_context.hpp"
#include "fmt/format.h"
#include "io/logging.h"
#include "net/message_handler.h"
#include <memory>

Net::Client::Client(u32 server_port, u32 client_port)
    : context(std::make_unique<asio::io_context>()),
      server_connection(*context, client_port), handler(nullptr) {
  asio::ip::udp::resolver resolver(*this->context);
  this->server_endpoint = *resolver
                               .resolve(asio::ip::udp::v4(), "127.0.0.1",
                                        fmt::format("{}", server_port))
                               .begin();
  server_connection.bind(this->server_endpoint);
}

void Net::Client::begin() {
  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->server_connection.listen();
  this->server_connection.write_connection_requested();
}

void Net::Client::register_callbacks(Net::MessageHandler *handler) {
  this->server_connection.register_callbacks(handler);
}

void Net::Client::shutdown() {
  this->context->stop();
  this->context_thread.join();
}

void Net::Client::ping_server() { this->server_connection.write_ping(); }

void Net::Client::send_inputs(const InputMap &inputs) {
  this->server_connection.write_user_inputs(inputs);
}
