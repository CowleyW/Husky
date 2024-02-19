#include "server_app.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "net/connection.h"
#include "net/message.h"
#include "net/server.h"
#include <memory>

ServerApp::ServerApp(u32 port)
    : server(std::make_unique<Net::Server>(port, 8)) {}

Err ServerApp::run() {
  this->server->register_callbacks(this);
  this->server->begin();

  this->running = true;

  while (this->running) {
    // do some stuff
  }

  return Err::ok();
}

void ServerApp::shutdown() {}

void ServerApp::on_connection_requested(const Net::Message &message,
                                        const asio::ip::udp::endpoint &remote) {
  if (message.body.size() != Net::Message::CONNECTION_REQUESTED_PADDING) {
    io::error(
        "Invalid ConnectionRequested padding: {} (actual) != {} (expected)",
        message.body.size(), Net::Message::CONNECTION_REQUESTED_PADDING);
    return;
  }
  io::debug("Received ConnectionRequested");

  Result<Net::Connection *const> result = this->server->get_client(remote);
  if (!result.is_error) {
    io::debug("Client already connected.");
    Net::Connection *const connection = result.value;
    connection->write_connection_accepted();
  } else if (this->server->has_open_client()) {
    this->server->accept(remote);
  } else {
    this->server->deny_connection(remote);
  }
}

void ServerApp::on_ping(const Net::Message &message) {
  io::debug("Received Ping");
}

void ServerApp::on_user_inputs(const Net::Message &message) {
  if (message.body.size() != InputMap::packed_size()) {
    io::error("Invalid message body size {} (actual) != {} (expected)",
              message.body.size(), InputMap::packed_size());
    return;
  }

  Result<InputMap> inputs_result = InputMap::deserialize(Buf<u8>(message.body));
  if (inputs_result.is_error) {
    io::error(inputs_result.msg);
    return;
  }

  InputMap inputs = inputs_result.value;

  io::debug("Received UserInputs: ({}, {}, {})", inputs.press_jump,
            inputs.press_left, inputs.press_right);
}
