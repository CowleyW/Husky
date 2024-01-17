#include "window.h"

#include "io/logging.h"

#include <GLFW/glfw3.h>

void Window::init() {
  if (!glfwInit()) {
    logging::console_fatal("Failed to initialize GLFW");
  }

  this->handle = glfwCreateWindow(1280, 720, "Triton", nullptr, nullptr);
  if (!this->handle) {
    logging::console_fatal("Failed to create a window.");
    this->shutdown();
    return;
  }

  glfwMakeContextCurrent(this->handle);
}

void Window::shutdown() {
  glfwDestroyWindow(this->handle);

  glfwTerminate();
}

void Window::loop() {
  while (!glfwWindowShouldClose(this->handle)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glfwSwapBuffers(this->handle);

    glfwPollEvents();
  }

  this->shutdown();
}
