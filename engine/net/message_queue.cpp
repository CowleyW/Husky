#include "message_queue.h"
#include "asio/error.hpp"
#include "io/logging.h"
#include "net/message.h"
Net::MessageQueue::MessageQueue(asio::ip::tcp::socket &socket)
    : socket(socket), queue(), buf(1024) {}

void Net::MessageQueue::begin() { this->begin_enqueue_next(); }

bool Net::MessageQueue::has_message() { return this->queue.size() > 0; }
Net::Message Net::MessageQueue::get_message() {
  Message message = this->queue.front();
  this->queue.pop();

  return message;
}

void Net::MessageQueue::begin_enqueue_next() {
  io::debug("Waiting to receive message.");

  auto read_callback = [this](std::error_code err, size_t len) {
    io::info("received some data: {} bytes.", len);
    if (err == asio::error::eof) {
      io::debug("Closing connection gracefully.");
      return;
    } else if (err) {
      io::error("{}", err.value());
    }

    // Probably not a good idea to just blindly read messages...
    // There are a couple possible issues,
    //   a) we might not get an entire message in one packet
    //   b) someone might create a bad message!
    // This will be fixed later
    this->queue.push(Message::deserialize_from(this->buf));

    this->begin_enqueue_next();
  };

  this->socket.async_read_some(asio::buffer(this->buf.data(), this->buf.size()),
                               read_callback);
}
