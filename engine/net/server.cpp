#include "server.h"

#include "io/logging.h"
#include "net/connection.h"
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

Result<Net::Connection *const>
Net::Server::get_client(const asio::ip::udp::endpoint &remote) {
  for (Connection &c : this->clients) {
    if (c.matches_remote(remote)) {
      return Result<Net::Connection *const>::ok(&c);
    }
  }

  return Result<Net::Connection *const>::err("No such connection");
}

bool Net::Server::has_open_client() {
  for (Connection &c : this->clients) {
    if (!c.is_connected()) {
      return true;
    }
  }

  return false;
}

void Net::Server::accept(const asio::ip::udp::endpoint &remote) {
  bool found_slot = false;

  for (Connection &c : this->clients) {
    if (!c.is_connected()) {
      c.bind(remote);
      c.write_connection_accepted();

      found_slot = true;
      break;
    }
  }

  if (!found_slot) {
    io::warn("Attempted to accept client when no slot was available.");
  }
}

void Net::Server::deny_connection(const asio::ip::udp::endpoint &remote) {
  Connection temp;
  temp.bind(remote);

  temp.write_connection_denied();
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
    this->handler->on_connection_requested(message, this->remote);
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
