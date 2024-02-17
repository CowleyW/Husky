#include "client.h"
#include "asio/io_context.hpp"
#include "fmt/format.h"
#include "io/logging.h"
#include "net/message_handler.h"
#include <memory>

Net::Client::Client(u32 server_port, u32 client_port)
    : context(std::make_unique<asio::io_context>()),
      server_connection(*context), handler(nullptr),
      listening_socket(
          *context, asio::ip::udp::endpoint(asio::ip::udp::v4(), client_port)) {
  asio::ip::udp::resolver resolver(*this->context);
  this->server_endpoint = *resolver
                               .resolve(asio::ip::udp::v4(), "127.0.0.1",
                                        fmt::format("{}", server_port))
                               .begin();
  server_connection.bind(this->server_endpoint);
}

void Net::Client::begin() {
  if (handler == nullptr) {
    io::fatal("Server has no bound message handler.");
    return;
  }

  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  this->start_receive();
  this->server_connection.write_connection_requested();
}

void Net::Client::register_callbacks(Net::MessageHandler *handler) {
  this->handler = handler;
}

void Net::Client::shutdown() {
  this->context->stop();
  this->context_thread.join();
}

void Net::Client::start_receive() {
  auto on_receive_callback = [this](const std::error_code &err, u64 size) {
    io::debug("received some bytes.");
    if (!err) {
      this->handle_receive(size);
    } else {
      io::error(err.message());
    }
  };
  this->listening_socket.async_receive_from(asio::buffer(this->recv_buf),
                                            this->remote, on_receive_callback);
}

void Net::Client::handle_receive(u64 size) {
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
