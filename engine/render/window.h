#pragma once

#include "io/input_map.h"
#include "render/callback_handler.h"
#include "render/vk_types.h"
#include "util/err.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

class Window {
public:
  ~Window();

  /**
   * Initializes the window.
   * @return true iff the window was successfully initialized, else false
   */
  Err init(Dimensions dimensions);

  void register_callbacks(CallbackHandler *callback);

  void swap_buffers();
  void poll_events();

  InputMap get_inputs();

  void *raw_window_handle();

  VkSurfaceKHR create_surface(VkInstance instance);

public:
  Dimensions dimensions;

private:
  GLFWwindow *handle;
};
