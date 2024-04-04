#pragma once

#include "application.h"
#include "asio/ip/udp.hpp"
#include "net/message_handler.h"
#include "net/server.h"
#include "util/err.h"
#include "world_state.h"

class ServerApp : public Application {
public:
  ServerApp(uint32_t port);

public:
  // Methods inherited from Application
  void begin() override;

  void update(float dt) override;

  void fixed_update() override;
  void shutdown() override;

private:
  void reset_process_mask();

  void handle_message(const Net::Message &message, uint8_t client_index);
  void poll_network();

  void handle_user_inputs(const Net::Message &message, uint8_t client_index);

private:
  static constexpr uint8_t MaxClients = 8;

  std::unique_ptr<Net::Server> server;
  bool running = false;

  std::vector<std::pair<uint8_t, InputMap>> client_inputs;
  std::array<bool, ServerApp::MaxClients> process_client_mask;

  uint32_t frame;
  WorldState world_state;
};
