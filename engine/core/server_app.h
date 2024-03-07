#pragma once

#include "application.h"
#include "asio/ip/udp.hpp"
#include "net/message_handler.h"
#include "net/server.h"
#include "types.h"
#include "util/err.h"

class ServerApp : public Application {
public:
  ServerApp(u32 port);

public:
  // Methods inherited from Application
  void begin() override;
  
  // The server doesn't need to do anything during update
  // void update() override;
  
  void fixed_update() override;
  void shutdown() override;

private:
  void handle_message(const Net::Message &message);
  void poll_network();

private:
  std::unique_ptr<Net::Server> server;
  bool running = false;
};
