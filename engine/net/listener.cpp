#include "listener.h"

#include "io/logging.h"

#include <asio.hpp>

Net::Listener::Listener(std::shared_ptr<asio::ip::udp::socket> socket)
    : socket(socket), handler(nullptr), recv_buf(1024) {}

Net::Listener::Listener(asio::io_context &context, u32 port)
    : socket(std::make_shared<asio::ip::udp::socket>(
          context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port))),
      handler(nullptr), recv_buf(1024) {}

void Net::Listener::register_callbacks(Net::MessageHandler* handler) {
  this->handler = handler;
}

void Net::Listener::listen() {
  if (!this->handler) {
    io::fatal("No message handler is set.");
    return;
  }
  
  auto on_receive = [this](const std::error_code &err, u64 size) {
    if (!err) {
      this->handle_receive((u32)size);
    } else {
      io::error("spot 2: {}", err.message());
    }
  };
  this->socket->async_receive_from(asio::buffer(this->recv_buf),
                                  this->recv_endpoint, on_receive);
}

void Net::Listener::handle_receive(u32 size) {
  Buf<u8> buf(this->recv_buf.data(), size);
  Err err = Net::verify_packet(buf);
  if (err.is_error) {
    io::error(err.msg);
    this->listen();
    return;
  }

  Buf<u8> trimmed_buf = buf.trim_left(Net::PacketHeader::packed_size());
  Result<Net::Message> result = Net::Message::deserialize(trimmed_buf);
  if (result.is_error) {
    io::error(result.msg);
    this->listen();
    return;
  }

  Net::Message message = result.value;

  switch (message.header.message_type) {
  case Net::MessageType::ConnectionRequested:
    this->handler->on_connection_requested(message, this->recv_endpoint);
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
  case Net::MessageType::UserInputs:
    this->handler->on_user_inputs(message);
    break;
  default:
    io::error("Unknown message type");
    break;
  }

  this->listen();
}