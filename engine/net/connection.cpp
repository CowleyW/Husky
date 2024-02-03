#include "connection.h"

#include "asio/error_code.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/write.hpp"
#include "io/logging.h"
#include "message.h"
#include "message_queue.h"

Net::Connection::Connection(asio::io_context &context)
    : socket(context), queue(socket) {}

void Net::Connection::write_message(Net::Message &message) {
  io::debug("...Sending message...");
  io::debug("Sequence ID -- {}", message.header.sequence_id);
  io::debug("Message ID -- {}", message.header.message_id);

  auto on_write_callback = [this](const asio::error_code &err, u64 len) {
    this->handle_write(err, len);
  };

  asio::async_write(this->socket, asio::buffer(message.serialize()),
                    on_write_callback);
}

void Net::Connection::handle_write(const asio::error_code &err, u64 len) {
  if (!err) {
    io::debug("Wrote {} bytes", len);
  } else {
    io::error("{}", err.message());
  }
}

void Net::Connection::begin_queue() { this->queue.begin(); }

void Net::Connection::shutdown() {
  try {
    this->socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    this->socket.close();
  } catch (std::exception &e) {
    // we don't care... just want to terminate connection
  }
}

bool Net::Connection::has_message() { return this->queue.has_message(); }

Net::Message Net::Connection::get_message() {
  return this->queue.get_message();
}
