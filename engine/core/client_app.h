#pragma once

#include "application.h"

#include "net/client.h"
#include "render/callback_handler.h"
#include "render/context.h"
#include "render/window.h"

class ClientApp : public Application, CallbackHandler, Net::MessageHandler {
public:
  ClientApp(u32 server_port, u32 client_port);

  Err run() override;
  void shutdown() override;

  Err init();

public:
  // Methods inherited from CallbackHandler
  void on_window_resize(Dimensions dimensions) override;
  void on_window_close() override;

public:
  // Methods inherited from MessageHandler
  void on_connection_accepted(const Net::Message &message) override;

  void on_connection_denied(const Net::Message &message) override;

  void on_ping(const Net::Message &message) override;

private:
  void network_update(const InputMap &inputs);

private:
  Window window;
  Context context;

  std::shared_ptr<Net::Client> client;

  bool running = false;
  u32 port;
};
