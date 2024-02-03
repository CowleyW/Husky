#include "server.h"
#include "asio/error_code.hpp"
#include "asio/io_context.hpp"
#include "io/logging.h"
#include "net/connection.h"
#include "net/message.h"
#include <memory>

Net::Server::Server(u32 port)
    : port(port), context(std::make_shared<asio::io_context>()),
      acceptor(*context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      clients() {}

void Net::Server::begin() {
  asio::io_context::work idle_work(*this->context);
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->start_accept();
}

void Net::Server::start_accept() {
  io::debug("Waiting for connection on port {}.", this->port);

  auto connection = std::make_shared<Connection>(*(this->context));

  auto on_accept = [=](const asio::error_code &err) {
    if (!err) {
      io::debug("Good connection.");
      this->handle_accept(connection);
    } else {
      io::error("{}", err.message());
    }
  };
  this->acceptor.async_accept(connection->socket, on_accept);
}

void Net::Server::handle_accept(std::shared_ptr<Connection> connection) {
  io::info("Client connected.");
  this->clients.emplace_back(connection);

  std::string text = "hello, user!";
  Net::Header header = {0, 0, 0, 0, 0, (u32)text.size()};
  Net::Message message = {header, std::vector<u8>(text.begin(), text.end())};
  connection->write_message(message);
  this->start_accept();
}
