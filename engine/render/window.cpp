#include "window.h"

#include "io/logging.h"
#include "render/callback_handler.h"
#include "render/gl_types.h"
#include "util/err.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdint>

Err Window::init(Dimensions dimensions) {
  this->dimensions = dimensions;

  if (!glfwInit()) {
    return Err::err("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  this->handle = glfwCreateWindow(this->dimensions.width, this->dimensions.height, "Triton", nullptr, nullptr);
  if (!this->handle) {
    this->shutdown();
    return Err::err("Failed to create a window.");
  }

  glfwMakeContextCurrent(this->handle);

  return Err::ok();
}

void Window::register_callbacks(CallbackHandler *callback) {
  glfwSetWindowUserPointer(this->handle, (void *) callback);

  glfwSetFramebufferSizeCallback(this->handle, [](GLFWwindow *window, int32_t width, int32_t height) {
    CallbackHandler *callback = static_cast<CallbackHandler *>(glfwGetWindowUserPointer(window));

    callback->on_window_resize({ (uint32_t)width, (uint32_t)height });
  });
}

void Window::shutdown() {
  if (this->handle != nullptr) {
    glfwDestroyWindow(this->handle);
  }

  glfwTerminate();
}

void Window::loop() {
  while (!glfwWindowShouldClose(this->handle)) {
    glClearColor(0.5, 0.8, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(this->handle);

    glfwPollEvents();
  }

  this->shutdown();
}

