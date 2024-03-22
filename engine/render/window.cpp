#include "window.h"

#include "io/logging.h"
#include "render/callback_handler.h"
#include "render/vk_types.h"
#include "util/err.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

Window::~Window() {
  if (this->handle != nullptr) {
    glfwDestroyWindow(this->handle);
  }

  glfwTerminate();
}

Err Window::init(Dimensions dimensions) {
  this->dimensions = dimensions;

  if (!glfwInit()) {
    return Err::err("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  this->handle = glfwCreateWindow(
      this->dimensions.width,
      this->dimensions.height,
      "Triton",
      nullptr,
      nullptr);
  if (!this->handle) {
    return Err::err("Failed to create a window.");
  }

  return Err::ok();
}

void Window::register_callbacks(CallbackHandler *callback) {
  glfwSetWindowUserPointer(this->handle, (void *)callback);

  glfwSetFramebufferSizeCallback(
      this->handle,
      [](GLFWwindow *window, int32_t width, int32_t height) {
        CallbackHandler *callback =
            static_cast<CallbackHandler *>(glfwGetWindowUserPointer(window));

        callback->on_window_resize({(uint32_t)width, (uint32_t)height});
      });

  glfwSetWindowCloseCallback(this->handle, [](GLFWwindow *window) {
    CallbackHandler *callback =
        static_cast<CallbackHandler *>(glfwGetWindowUserPointer(window));

    callback->on_window_close();
  });
}

void Window::swap_buffers() {
  // glfwSwapBuffers(this->handle);
}

void Window::poll_events() {
  glfwPollEvents();
}

InputMap Window::get_inputs() {
  bool jump = glfwGetKey(this->handle, GLFW_KEY_SPACE);
  bool left = glfwGetKey(this->handle, GLFW_KEY_A);
  bool right = glfwGetKey(this->handle, GLFW_KEY_D);

  return {jump, left, right};
}

VkSurfaceKHR Window::create_surface(VkInstance instance) {
  VkSurfaceKHR surface;
  VkResult err =
      glfwCreateWindowSurface(instance, this->handle, nullptr, &surface);

  if (err) {
    io::error("Failed to create window surface.");
  }

  return surface;
}
