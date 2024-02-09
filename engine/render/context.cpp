#include "context.h"
#include "core/types.h"
#include "gl_types.h"
#include "shader.h"
#include "util/err.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

Err Context::init(Dimensions dimensions) {
  i32 result = gladLoadGL(glfwGetProcAddress);
  if (!result) {
    return Err::err("Failed to initialize OpenGL context");
  }

  this->resize(dimensions);

  // Vertex Array
  glGenVertexArrays(1, &this->vertex_array_id);
  glBindVertexArray(this->vertex_array_id);

  // Vertex Buffer
  float vertices[] = {
      0.5f,  0.5f,  0.0f, // top right
      0.5f,  -0.5f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, // bottom left
      -0.5f, 0.5f,  0.0f  // top left
  };
  glGenBuffers(1, &this->vbuf_id);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbuf_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Index Buffer
  u32 indices[] = {
      0, 1, 3, // first triangle
      1, 2, 3  // second triangle
  };
  glGenBuffers(1, &this->ibuf_id);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ibuf_id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
               GL_STATIC_DRAW);

  Err err = this->shader.init("shaders/standard_vert.glsl",
                              "shaders/standard_frag.glsl");
  if (err.is_error) {
    return err;
  }

  // Link vertex attributes
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);

  return Err::ok();
}

void Context::resize(Dimensions dimensions) {
  glViewport(0, 0, dimensions.width, dimensions.height);
}

void Context::clear() {
  glClearColor(0.5, 0.8, 0.2, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void Context::render() {
  this->shader.bind();
  glBindVertexArray(this->vertex_array_id);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
