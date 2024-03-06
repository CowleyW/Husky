#include "server_app.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "net/message.h"
#include "net/server.h"

#include <memory>

ServerApp::ServerApp(u32 port)
    : server(std::make_unique<Net::Server>(port, 8)) {}

Err ServerApp::run() {
  this->server->begin();

  this->running = true;

  // Time in nanoseconds
  using namespace std::literals::chrono_literals;
  constexpr std::chrono::nanoseconds dt(100ms);

  std::chrono::nanoseconds accumulator(0ns);
  auto prev_time = std::chrono::steady_clock::now();

  while (this->running) {
    auto now = std::chrono::steady_clock::now();
    auto frame_time = now - prev_time;
    prev_time = now;

    accumulator += frame_time;

    while (accumulator >= dt) {
      accumulator -= dt;

      this->poll_network();
      this->server->ping_all();
    }
  }

  return Err::ok();
}

void ServerApp::shutdown() {}

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