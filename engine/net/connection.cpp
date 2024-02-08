#include "connection.h"

#include "io/logging.h"
#include "net/message.h"
#include "util/err.h"

Net::Connection::Connection() : connected(false), remote_id(0) {}

void Net::Connection::free() {
  this->connected = false;
  this->remote_id = 0;
}

Err Net::Connection::bind(u32 remote_id) {
  if (this->connected) {
    return Err::err("Client is already connected to this slot");
  }

  this->connected = true;
  this->remote_id = remote_id;

  return Err::ok();
}

bool Net::Connection::is_connected() { return this->connected; }

bool Net::Connection::matches_id(u32 id) { return this->remote_id == id; }

void Net::Connection::write_message(const Net::Message &message) {
  io::debug("TODO: Write message to {}", this->remote_id);
}
