#pragma once

#include "render/gl_types.h"

class CallbackHandler {
public:
  virtual void on_window_resize(Dimensions dimensions) = 0;
};
