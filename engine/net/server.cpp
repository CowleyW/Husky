#include "server.h"

#include "io/logging.h"
#include <system_error>

Net::Server::Server(uint32_t port, uint8_t max_clients)
    : port(port), max_clients(max_clients), num_connected_clients(0),
      clients(max_clients), context(std::make_unique<asio::io_context>()),
      socket(*context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)) {
  for (Connection &client : this->clients) {
    client.free();
  }
}

void Net::Server::begin() {
  io::debug("Beginning server.");

  this->context_thread = std::thread([this]() { this->context->run(); });

  this->start_receive();
}

void Net::Server::start_receive() {
  auto on_receive_callback = [this](const std::error_code &err, u64 size) {
    if (!err) {
      io::debug("Received {} bytes!", size);
      this->handle_receive(size);
    } else {
      io::error(err.message());
    }
  };
  this->socket.async_receive_from(asio::buffer(this->recv_buf), this->remote,
                                  on_receive_callback);
}

void Net::Server::handle_receive(u64 size) {
  io::debug("\"{}\"", (char *)this->recv_buf.data());
  this->start_receive();
}
