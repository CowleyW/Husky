#pragma once

#include "io/raw_inputs.h"
#include "render/vk_types.h"

class CallbackHandler {
public:
  virtual void on_window_resize(Dimensions dimensions) = 0;

  virtual void on_window_close() = 0;

  virtual void on_mouse_move(double x, double y) = 0;

  virtual void on_mouse_button(MouseButton button, int32_t action) = 0;

  virtual void on_key_event(KeyCode key, int32_t action) = 0;
};
