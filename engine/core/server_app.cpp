#include "server_app.h"
#include "io/logging.h"
#include "net/connection.h"
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

void ServerApp::on_connection_accepted(const Net::Message &message) {
  io::debug("Received ConnectionAccepted");
}

void ServerApp::on_connection_denied(const Net::Message &message) {
  io::debug("Received ConnectionDenied");
}

void ServerApp::on_ping(const Net::Message &message) {
  io::debug("Received Ping");
}
