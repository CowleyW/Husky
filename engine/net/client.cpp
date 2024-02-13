#include "client.h"
#include "asio/io_context.hpp"
#include "core/def.h"
#include "crypto/checksum.h"
#include "fmt/format.h"
#include "io/logging.h"
#include "net/message.h"
#include "net/message_builder.h"
#include "util/serialize.h"
#include <memory>

Net::Client::Client(u32 port)
    : context(std::make_unique<asio::io_context>()), socket(*context) {
  asio::ip::udp::resolver resolver(*this->context);
  this->server_endpoint =
      *resolver
           .resolve(asio::ip::udp::v4(), "127.0.0.1", fmt::format("{}", port))
           .begin();
  socket.open(asio::ip::udp::v4());
}

void Net::Client::begin() {
  io::debug("Beginning client.");
  this->context_thread = std::thread([this]() { this->context->run(); });

  MessageBuilder builder(Net::MessageType::ConnectionRequested);
  Message message = builder.with_ids(0x00, 0x00, 0x00)
                        .with_acks(0, 0)
                        .with_padding(512)
                        .build();
  io::debug("Message size: {}", message.packed_size());
  io::debug("Body size: {}", message.header.body_size);

  std::vector<u8> buf(message.packed_size() + PacketHeader::packed_size());
  message.serialize_into(buf, PacketHeader::packed_size());

  u32 offset = Serialize::serialize_u32(NET_PROTOCOL_ID, buf, 0);
  u32 crc = Crypto::calculate_checksum(&buf[PacketHeader::packed_size()],
                                       message.packed_size());
  Serialize::serialize_u32(crc, buf, offset);

  this->socket.send_to(asio::buffer(buf), this->server_endpoint);
}

void Net::Client::shutdown() {
  this->context->stop();
  this->context_thread.join();
}
