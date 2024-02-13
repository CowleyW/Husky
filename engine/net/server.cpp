#include "server.h"

#include "io/logging.h"
#include "net/message.h"
#include "net/message_handler.h"
#include <system_error>

Net::Server::Server(uint32_t port, uint8_t max_clients)
    : port(port), max_clients(max_clients), num_connected_clients(0),
      clients(max_clients), context(std::make_unique<asio::io_context>()),
      socket(*context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
      recv_buf(1024), handler(nullptr) {
  for (Connection &client : this->clients) {
    client.free();
  }
}

void Net::Server::begin() {
  if (handler == nullptr) {
    io::fatal("Server has no bound message handler.");
    return;
  }
  io::debug("Beginning server.");

  this->context_thread = std::thread([this]() { this->context->run(); });

  this->start_receive();
}

void Net::Server::register_callbacks(Net::MessageHandler *handler) {
  this->handler = handler;
}

void Net::Server::start_receive() {
  auto on_receive_callback = [this](const std::error_code &err, u64 size) {
    if (!err) {
      this->handle_receive(size);
    } else {
      io::error(err.message());
    }
  };
  this->socket.async_receive_from(asio::buffer(this->recv_buf), this->remote,
                                  on_receive_callback);
}

void Net::Server::handle_receive(u64 size) {
  Buf<u8> buf(this->recv_buf.data(), (u32)size);
  Err err = Net::verify_packet(buf);
  if (err.is_error) {
    io::error(err.msg);
    this->start_receive();
    return;
  }

  Buf<u8> trimmed_buf = buf.trim_left(Net::PacketHeader::packed_size());
  Result<Net::Message> result = Net::Message::deserialize(trimmed_buf);
  if (result.is_error) {
    io::error(result.msg);
    this->start_receive();
    return;
  }

  Net::Message message = result.value;

  switch (message.header.message_type) {
  case Net::MessageType::ConnectionRequested:
    this->handler->on_connection_requested(message);
    break;

  case Net::MessageType::ConnectionAccepted:
    this->handler->on_connection_accepted(message);
    break;

  case Net::MessageType::ConnectionDenied:
    this->handler->on_connection_denied(message);
    break;

  case Net::MessageType::Ping:
    this->handler->on_ping(message);
    break;

  default:
    io::error("Unknown message type");
    break;
  }

  this->start_receive();
}
