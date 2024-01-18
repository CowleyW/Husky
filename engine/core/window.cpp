#include "window.h"

#include "io/logging.h"
#include "util/err.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <cstdint>

Err Window::init() {
  if (!glfwInit()) {
    return Err::err("Failed to initialize GLFW");
  }

  this->handle = glfwCreateWindow(1280, 720, "Triton", nullptr, nullptr);
  if (!this->handle) {
    this->shutdown();
    return Err::err("Failed to create a window.");
  }

  glfwMakeContextCurrent(this->handle);

  // All this code should be moved out later!

  int32_t version = gladLoadGL(glfwGetProcAddress);
  if (version == 0) {
    this->shutdown();
    return Err::err("Failed to initialize OpenGL context");
  }

  // All this code should be moved out later!
  return Err::ok();
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
