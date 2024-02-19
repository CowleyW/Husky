#pragma once

#include "io/input_map.h"
#include "render/callback_handler.h"
#include "render/gl_types.h"
#include "util/err.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window {
public:
  /**
   * Initializes the window.
   * @return true iff the window was successfully initialized, else false
   */
  Err init(Dimensions dimensions);

  void register_callbacks(CallbackHandler *callback);

  /**
   * Shuts down the window and performs the appropriate cleanup.
   */
  void shutdown();

  void swap_buffers();
  void poll_events();

  InputMap build_input_map();

public:
  Dimensions dimensions;

private:
  GLFWwindow *handle;
};
