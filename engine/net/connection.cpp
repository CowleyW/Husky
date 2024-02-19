#include "connection.h"

#include "core/def.h"
#include "crypto/checksum.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "net/message.h"
#include "net/message_builder.h"
#include "net/message_handler.h"
#include "util/err.h"
#include "util/serialize.h"

#include <random>
#include <vector>

Net::Connection::Connection(asio::io_context &context)
    : connected(false), remote_id(0), socket(context), send_buf(),
      recv_buf(1024), handler(nullptr) {
  socket.open(asio::ip::udp::v4());
}

Net::Connection::Connection(asio::io_context &context, u32 port)
    : connected(false), remote_id(0),
      socket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)),
      send_buf(), recv_buf(1024), handler(nullptr) {}

void Net::Connection::free() {
  this->connected = false;
  this->remote_id = 0;
}

Err Net::Connection::bind(const asio::ip::udp::endpoint &send_endpoint) {
  if (this->connected) {
    return Err::err("Client is already connected to this slot");
  }

  this->connected = true;

  // Create random remote_id -- probably shouldn't happen here. The server
  // should be in control of ids and should supply them to the clients
  std::random_device device;
  std::mt19937 rng(device());
  std::uniform_int_distribution<u32> distrib;
  this->remote_id = distrib(rng);
  io::debug("New remote id {}", this->remote_id);

  this->send_endpoint = send_endpoint;

  return Err::ok();
}

void Net::Connection::register_callbacks(Net::MessageHandler *handler) {
  this->handler = handler;
}

void Net::Connection::listen() {
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
  this->socket.async_receive_from(asio::buffer(this->recv_buf),
                                  this->recv_endpoint, on_receive);
}

void Net::Connection::handle_receive(u32 size) {
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

Net::MessageBuilder Net::Connection::message_scaffold(Net::MessageType type) {
  return Net::MessageBuilder(type)
      .with_ids(this->remote_id, this->sequence_id, this->message_id)
      .with_acks(this->ack, this->ack_bitfield);
}

bool Net::Connection::is_connected() { return this->connected; }

bool Net::Connection::matches_id(u32 id) { return this->remote_id == id; }

bool Net::Connection::matches_remote(const asio::ip::udp::endpoint &remote) {
  return this->send_endpoint == remote;
}

void Net::Connection::write_message(const Net::Message &message) {
  u32 message_size = message.packed_size() + Net::PacketHeader::packed_size();
  this->send_buf.resize(message_size);
  message.serialize_into(this->send_buf, PacketHeader::packed_size());

  // Serialize the protocol ID
  u32 offset = Serialize::serialize_u32(NET_PROTOCOL_ID, this->send_buf, 0);

  // Calculate and serialize the checksum
  u32 crc = Crypto::calculate_checksum(
      &this->send_buf[PacketHeader::packed_size()], message.packed_size());
  Serialize::serialize_u32(crc, this->send_buf, offset);

  auto on_send = [this](const asio::error_code &err, u64 size) {
    if (err) {
      io::error(err.message());
      return;
    }

    io::debug("Sent {} bytes to {}:{}.", size,
              this->send_endpoint.address().to_string(),
              this->send_endpoint.port());
  };
  this->socket.async_send_to(asio::buffer(this->send_buf), this->send_endpoint,
                             on_send);
}

void Net::Connection::write_connection_requested() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionRequested)
          .with_padding(Net::Message::CONNECTION_REQUESTED_PADDING)
          .build();

  this->write_message(message);
}

void Net::Connection::write_connection_accepted() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionAccepted).build();

  this->write_message(message);
}

void Net::Connection::write_connection_denied() {
  Net::Message message =
      this->message_scaffold(Net::MessageType::ConnectionDenied).build();

  this->write_message(message);
}

void Net::Connection::write_ping() {
  Net::Message message = this->message_scaffold(Net::MessageType::Ping).build();

  this->write_message(message);
}

void Net::Connection::write_user_inputs(const InputMap &inputs) {
  std::vector<u8> input_map(InputMap::packed_size());
  inputs.serialize_into(input_map, 0);

  Net::Message message = this->message_scaffold(Net::MessageType::UserInputs)
                             .with_body(input_map)
                             .build();

  this->write_message(message);
}
