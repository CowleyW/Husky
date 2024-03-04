#include "window.h"

#include "core/types.h"
#include "render/callback_handler.h"
#include "render/gl_types.h"
#include "util/err.h"
#include "io/logging.h"

#include <GLFW/glfw3.h>
#include <glad/gl.h>

Err Window::init(Dimensions dimensions) {
  this->dimensions = dimensions;

  if (!glfwInit()) {
    return Err::err("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  this->handle =
      glfwCreateWindow(this->dimensions.width, this->dimensions.height,
                       "Triton", nullptr, nullptr);
  if (!this->handle) {
    this->shutdown();
    return Err::err("Failed to create a window.");
  }

  glfwMakeContextCurrent(this->handle);

  return Err::ok();
}

void Window::register_callbacks(CallbackHandler *callback) {
  glfwSetWindowUserPointer(this->handle, (void *)callback);

  glfwSetFramebufferSizeCallback(
      this->handle, [](GLFWwindow *window, i32 width, i32 height) {
        CallbackHandler *callback =
            static_cast<CallbackHandler *>(glfwGetWindowUserPointer(window));

        callback->on_window_resize({(u32)width, (u32)height});
      });

  glfwSetWindowCloseCallback(this->handle, [](GLFWwindow *window) {
    CallbackHandler *callback =
        static_cast<CallbackHandler *>(glfwGetWindowUserPointer(window));

    callback->on_window_close();
  });
}

void Window::shutdown() {
  if (this->handle != nullptr) {
    glfwDestroyWindow(this->handle);
  }

  glfwTerminate();
}

void Window::swap_buffers() { glfwSwapBuffers(this->handle); }

void Window::poll_events() { glfwPollEvents(); }

InputMap Window::build_input_map() {
  bool jump = glfwGetKey(this->handle, GLFW_KEY_SPACE);
  bool left = glfwGetKey(this->handle, GLFW_KEY_A);
  bool right = glfwGetKey(this->handle, GLFW_KEY_D);

  return {jump, left, right};
}
