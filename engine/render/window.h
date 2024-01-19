#pragma once

#include "render/gl_types.h"
#include "render/context.h"
#include "render/callback_handler.h"
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

public:
  Dimensions dimensions;

private:
  GLFWwindow *handle;
};