#pragma once

#include "util/err.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window {
public:

  /**
   * Initializes the window.
   * @return true iff the window was successfully initialized, else false
   */
  Err init();

  /**
   * Shuts down the window and performs the appropriate cleanup.
   */
  void shutdown();

  /**
   * temp?
   */
  void loop();

private:
  GLFWwindow *handle;
};
