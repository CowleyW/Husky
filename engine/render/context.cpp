#include "context.h"
#include "util/err.h"
#include "gl_types.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

Err Context::init(Dimensions dimensions) {
  int32_t result = gladLoadGL(glfwGetProcAddress);
  if (result == 0) {
    return Err::err("Failed to initialize OpenGL context");
  }

  this->resize(dimensions);

  return Err::ok();
}

void Context::resize(Dimensions dimensions) {
  glViewport(0, 0, dimensions.width, dimensions.height);
}
