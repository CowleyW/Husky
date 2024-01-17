#pragma once

#include <GLFW/glfw3.h>

class Window {
public:
  void init();
  void shutdown();
  void loop();

private:
  GLFWwindow *handle;
};
