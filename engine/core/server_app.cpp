#include "server_app.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "net/message.h"
#include "net/server.h"
#include "world_state.h"

#include <memory>

ServerApp::ServerApp(u32 port)
    : server(std::make_unique<Net::Server>(port, ServerApp::MaxClients)),
      frame(0), process_client_mask(), world_state() {
  io::debug("world state size {}", this->world_state.packed_size());
  std::vector<u8> buf(this->world_state.packed_size());
  this->world_state.serialize_into(buf, 0);
}

void ServerApp::begin() { this->server->begin(); }

void ServerApp::update() {
  this->poll_network();

  auto dc_client = this->server->next_disconnected_client();
  while (dc_client.has_value()) {
    io::debug("User {} disconnected :(", dc_client.value());
    dc_client = this->server->next_disconnected_client();
  }

  auto new_client = this->server->next_new_client();
  while (new_client.has_value()) {
    io::debug("New client {} :)", new_client.value());
    new_client = this->server->next_new_client();
  }
}

void ServerApp::fixed_update() {
  this->frame += 1;

  for (auto &pair : this->client_inputs) {
    InputMap map = pair.second;
    io::debug("Received inputs from [{}] : ({}, {}, {})", pair.first,
              map.press_left, map.press_right, map.press_jump);
  }

  if (this->frame % 6 == 0) {
    this->server->send_world_state(this->world_state);
    this->frame = 0;
  }

  this->client_inputs.clear();
  this->reset_process_mask();
}

void ServerApp::shutdown() { this->server->shutdown(); }

void ServerApp::reset_process_mask() {
  for (u32 i = 0; i < this->MaxClients; i += 1) {
    this->process_client_mask[i] = false;
  }
}

void ServerApp::handle_message(const Net::Message &message, u8 client_index) {
  io::debug("[s_id: {}] [ack: {} | bits: {:b}]", message.header.sequence_id,
            message.header.ack, message.header.ack_bitfield);
  switch (message.header.message_type) {
  case Net::MessageType::ConnectionRequested:
    io::debug("Connection requested from {}.", message.header.salt);
    break;
  case Net::MessageType::Ping:
    io::debug("Received ping from {}.", message.header.salt);
    break;
  case Net::MessageType::UserInputs: {
    this->handle_user_inputs(message, client_index);
    break;
  }
  default:
    io::error("Unknown message type {}", (u8)message.header.message_type);
    break;
  }
}

void ServerApp::poll_network() {
  for (auto &client : this->server->get_clients()) {
    // If the client is not connected OR we have already processed the current
    // client then we skip over them. We poll the network every frame, but only
    // want to act on the data we receive during fixed_update
    if (!client.is_connected() || this->process_client_mask[client.index()]) {
      continue;
    }

    auto maybe = client.next_message();
    if (maybe.has_value()) {
      this->handle_message(maybe.value(), client.index());

      // Now that we've processed the client we mark it as seen
      this->process_client_mask[client.index()] = true;
    } else if (client.maybe_timeout()) {
      // Maybe the client has timed out if we didn't get a message from them?
      io::debug("client timed out");
    }
  }
}

void ServerApp::handle_user_inputs(const Net::Message &message,
                                   u8 client_index) {
  auto result = InputMap::deserialize(Buf<u8>(message.body));
  if (result.is_error) {
    io::error("Failed to read inputs from {}", message.header.salt);
  } else {
    InputMap map = result.value;

    this->client_inputs.push_back({client_index, map});
  }
}