#pragma once

#include "application.h"

#include "io/raw_inputs.h"
#include "net/client.h"
#include "render/callback_handler.h"
#include "render/vk_engine.h"
#include "world_state.h"

class ClientApp : public Application, CallbackHandler {
public:
  ClientApp(uint32_t server_port, uint32_t client_port);

  Err init();

public:
  // Methods inherited from Application
  void begin() override;
  void update() override;
  void fixed_update() override;
  void render() override;
  void shutdown() override;

public:
  // Methods inherited from CallbackHandler
  void on_window_resize(Dimensions dimensions) override;
  void on_window_close() override;
  void on_mouse_move(double x, double y) override;
  void on_key_event(int32_t key, int32_t action) override;
  void on_mouse_button(int32_t key, int32_t action) override;

private:
  void on_connection_accepted(const Net::Message &message);
  void on_connection_denied(const Net::Message &message);
  void on_ping(const Net::Message &message);
  void on_world_snapshot(const WorldState &world_state);

  void network_update(const InputMap &inputs);
  void poll_network();
  void handle_message(const Net::Message &message);

private:
  Render::VulkanEngine render_engine{};
  RawInputs inputs;
  Scene scene;

  std::shared_ptr<Net::Client> client;

  std::optional<uint32_t> client_index;

  bool running = false;
  uint32_t frame;
  WorldState world_state;
};
