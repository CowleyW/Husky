#pragma once

#include "render/callback_handler.h"
#include "render/window.h"
#include "render/context.h"
#include "util/err.h"

class Application : CallbackHandler {
public:
  Err init();
  Err run();
  void shutdown();

public:
  void on_window_resize(Dimensions dimensions) override;
  void on_window_close() override;

private:
  Window window;
  Context context;
  
  bool running = false;
};
