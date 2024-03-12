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

  void update() override;

  void fixed_update() override;
  void shutdown() override;

private:
  void reset_process_mask();

  void handle_message(const Net::Message &message, u8 client_index);
  void poll_network();

  void handle_user_inputs(const Net::Message &message, u8 client_index);

private:
  static constexpr u8 MaxClients = 8;

  std::unique_ptr<Net::Server> server;
  bool running = false;

  std::vector<std::pair<u8, InputMap>> client_inputs;
  std::array<bool, ServerApp::MaxClients> process_client_mask;

  u32 frame;
};
