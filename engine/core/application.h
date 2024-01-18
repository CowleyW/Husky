#pragma once

#include "core/window.h"
#include "util/err.h"

class Application {
public:
  Err init();
  Err run();
  void shutdown();
private:
  Window window;
};
