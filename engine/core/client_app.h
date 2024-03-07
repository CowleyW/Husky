#pragma once

#include "application.h"

#include "net/client.h"
#include "render/callback_handler.h"
#include "render/context.h"
#include "render/window.h"

class ClientApp : public Application, CallbackHandler {
public:
  ClientApp(u32 server_port, u32 client_port);

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

private:
  void on_connection_accepted(const Net::Message &message);

  void on_connection_denied(const Net::Message &message);

  void on_ping(const Net::Message &message);

  void network_update(const InputMap &inputs);
  void poll_network();
  void handle_message(const Net::Message &message);

private:
  Window window;
  Context context;

  std::shared_ptr<Net::Client> client;

  bool running = false;
};
