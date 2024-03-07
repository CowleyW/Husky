#include "client_app.h"

#include "io/input_map.h"
#include "io/logging.h"
#include "render/window.h"
#include "util/err.h"
#include "util/serialize.h"

#include <chrono>

ClientApp::ClientApp(u32 server_port, u32 client_port)
    : client(std::make_shared<Net::Client>(server_port, client_port)) {}

Err ClientApp::init() {
  Err err = this->window.init({1280, 720});
  if (err.is_error) {
    return err;
  }

  err = this->context.init(this->window.dimensions);
  if (err.is_error) {
    return err;
  }

  this->window.register_callbacks(this);

  return Err::ok();
}

void ClientApp::begin() { this->client->begin(); }

void ClientApp::update() { this->window.poll_events(); }

void ClientApp::render() {
  this->context.clear();
  this->context.render();
  this->window.swap_buffers();
}

void ClientApp::fixed_update() {
  InputMap inputs = this->window.build_input_map();

  this->poll_network();
  this->network_update(inputs);
}

void ClientApp::shutdown() {
  this->window.shutdown();
  this->client->shutdown();
}

void ClientApp::on_window_resize(Dimensions dimensions) {
  io::debug("Resizing window");
  this->context.resize(dimensions);
}

void ClientApp::on_window_close() {
  io::debug("Closing window");
  this->stop();
}

void ClientApp::on_connection_accepted(const Net::Message &message) {
  io::debug("Received ConnectionAccepted");
}

void ClientApp::on_connection_denied(const Net::Message &message) {
  io::debug("Received ConnectionDenied");
}

void ClientApp::on_ping(const Net::Message &message) {
  io::debug("Received Ping");
}

void ClientApp::network_update(const InputMap &inputs) {
  if (this->client->is_connected()) {
    this->client->send_inputs(inputs);
  }
}

void ClientApp::poll_network() {
  while (true) {
    auto maybe = this->client->next_message();

    if (maybe.has_value()) {
      this->handle_message(maybe.value());
    } else {
      break;
    }
  }
}

void ClientApp::handle_message(const Net::Message &message) {
  io::debug("[s_id: {}] [ack: {} | bits: {:b}]", message.header.sequence_id,
            message.header.ack, message.header.ack_bitfield);
  switch (message.header.message_type) {
  case Net::MessageType::ConnectionAccepted:
    this->on_connection_accepted(message);
    break;
  case Net::MessageType::ConnectionDenied:
    this->on_connection_denied(message);
    break;
  case Net::MessageType::Ping:
    this->on_ping(message);
    break;
  default:
    io::error("Unknown message type {}", (u8)message.header.message_type);
    break;
  }
}
