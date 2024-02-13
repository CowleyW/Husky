#pragma once

#include "application.h"
#include "asio/ip/udp.hpp"
#include "net/message_handler.h"
#include "net/server.h"
#include "types.h"
#include "util/err.h"

class ServerApp : public Application, Net::MessageHandler {
public:
  ServerApp(u32 port);

public:
  // Methods inherited from Application
  Err run() override;
  void shutdown() override;

public:
  // Methods inherited from MessageHandler
  void on_connection_requested(const Net::Message &message,
                               const asio::ip::udp::endpoint &remote) override;

  void on_connection_accepted(const Net::Message &message) override;

  void on_connection_denied(const Net::Message &message) override;

  void on_ping(const Net::Message &message) override;

private:
  std::unique_ptr<Net::Server> server;
  bool running = false;
};
