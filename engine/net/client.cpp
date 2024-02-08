#include "client.h"
#include "asio/io_context.hpp"
#include "fmt/format.h"
#include "io/logging.h"
#include <memory>

Net::Client::Client(u32 port)
    : context(std::make_unique<asio::io_context>()), socket(*context) {
  asio::ip::udp::resolver resolver(*this->context);
  this->server_endpoint =
      *resolver
           .resolve(asio::ip::udp::v4(), "127.0.0.1", fmt::format("{}", port))
           .begin();
  socket.open(asio::ip::udp::v4());
}

void Net::Client::begin() {
  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->socket.send_to(asio::buffer("Hello, Server!\0", 15),
                       this->server_endpoint);
}

void Net::Client::shutdown() {
  this->context->stop();
  this->context_thread.join();
}
