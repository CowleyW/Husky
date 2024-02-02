#include "server_app.h"
#include "asio/io_context.hpp"
#include "net/server.h"
#include <memory>

ServerApp::ServerApp(u32 port) : server(std::make_unique<Net::Server>(port)) {}

Err ServerApp::run() {
  this->server->begin();

  this->running = true;

  while (this->running) {
    // do some stuff
  }

  return Err::ok();
}

void ServerApp::shutdown() {}

void start_accept() {}
