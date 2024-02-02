#include "client.h"
#include "asio/connect.hpp"
#include "asio/io_context.hpp"
#include "fmt/format.h"

Net::Client::Client(u32 port)
    : port(port), context(std::make_shared<asio::io_context>()),
      server_connection(std::make_shared<Net::Connection>(*context)) {}

void Net::Client::begin() {
  asio::io_context::work idle_work(*this->context);
  this->context_thread = std::thread([this]() { this->context->run(); });

  asio::ip::tcp::resolver resolver(*this->context);
  auto endpoints = resolver.resolve("127.0.0.1", fmt::format("{}", this->port));
  asio::connect(this->server_connection->socket, endpoints);
}
