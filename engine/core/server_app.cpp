#include "server_app.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "net/message.h"
#include "net/server.h"

#include <memory>

ServerApp::ServerApp(u32 port)
    : server(std::make_unique<Net::Server>(port, 8)) {}

void ServerApp::begin() { this->server->begin(); }

void ServerApp::fixed_update() {
  this->poll_network();
  this->server->ping_all();
}

void ServerApp::shutdown() { this->server->shutdown(); }

void ServerApp::handle_message(const Net::Message &message) {
  io::debug("[s_id: {}] [ack: {} | bits: {:b}]", message.header.sequence_id,
            message.header.ack, message.header.ack_bitfield);
  switch (message.header.message_type) {
  case Net::MessageType::Ping:
    io::debug("Received ping from {}.", message.header.remote_id);
    break;
  case Net::MessageType::ClientInputs: {
    auto result = InputMap::deserialize(Buf<u8>(message.body));
    if (result.is_error) {
      io::error("Failed to read inputs from {}", message.header.remote_id);
    } else {
      InputMap map = result.value;
      io::debug("Received inputs from {} : ({}, {}, {})",
                message.header.remote_id, map.press_left, map.press_right,
                map.press_jump);
    }
    break;
  }
  default:
    io::error("Unknown message type {}", (u8)message.header.message_type);
    break;
  }
}

void ServerApp::poll_network() {
  for (auto &client : this->server->get_clients()) {
    auto maybe = client.next_message();
    if (maybe.has_value()) {
      this->handle_message(maybe.value());
    }
  }
}