#pragma once

#include "application.h"

#include "net/client.h"
#include "render/callback_handler.h"
#include "render/context.h"
#include "render/window.h"
#include <memory>

class ClientApp : public Application, CallbackHandler {
public:
  ClientApp(u32 port);

  Err run() override;
  void shutdown() override;

  Err init();

public:
  void on_window_resize(Dimensions dimensions) override;
  void on_window_close() override;

private:
  Window window;
  Context context;

  std::shared_ptr<Net::Client> client;

  bool running = false;
  u32 port;
};
