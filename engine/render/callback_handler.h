#pragma once

#include "render/vk_types.h"

class CallbackHandler {
public:
  virtual void on_window_resize(Dimensions dimensions) = 0;

  virtual void on_window_close() = 0;
};
