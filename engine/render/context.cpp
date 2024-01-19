#include "context.h"
#include "gl_types.h"
#include "util/err.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

Err Context::init(Dimensions dimensions) {
  int32_t result = gladLoadGL(glfwGetProcAddress);
  if (!result) {
    return Err::err("Failed to initialize OpenGL context");
  }

  this->resize(dimensions);

  // Vertex Array
  glGenVertexArrays(1, &this->vertex_array_id);
  glBindVertexArray(this->vertex_array_id);

  // Vertex Buffer
  float vertices[] = {-0.5f, -0.5f, 0.0f, 0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f};

  glGenBuffers(1, &this->vbuf_id);
  glBindBuffer(GL_ARRAY_BUFFER, this->vbuf_id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Vertex Shader
  const char *vertexShaderSource =
      "#version 330 core\n"
      "layout (location = 0) in vec3 aPos;\n"
      "void main()\n"
      "{\n"
      "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
      "}\0";
  this->vshader_id = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(this->vshader_id, 1, &vertexShaderSource, nullptr);
  glCompileShader(this->vshader_id);

  glGetShaderiv(this->vshader_id, GL_COMPILE_STATUS, &result);
  if (!result) {
    return Err::err("Failed to compile vertex shader");
  }

  // Fragment Shader
  const char *fragmentShaderSource =
      "#version 330 core\n"
      "out vec4 FragColor;\n"
      "void main()"
      "{\n"
      "    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
      "}\0";
  this->fshader_id = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(this->fshader_id, 1, &fragmentShaderSource, nullptr);
  glCompileShader(this->fshader_id);

  glGetShaderiv(this->fshader_id, GL_COMPILE_STATUS, &result);
  if (!result) {
    return Err::err("Failed to compile fragment shader");
  }

  // Shader Program
  this->program_id = glCreateProgram();
  glAttachShader(this->program_id, this->vshader_id);
  glAttachShader(this->program_id, this->fshader_id);
  glLinkProgram(this->program_id);

  glGetProgramiv(this->program_id, GL_LINK_STATUS, &result);
  if (!result) {
    return Err::err("Failed to link shaders");
  }

  // Cleanup
  glDeleteShader(this->vshader_id);
  glDeleteShader(this->fshader_id);

  // Use the shader program
  glUseProgram(this->program_id);

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
  glUseProgram(this->program_id);
  glBindVertexArray(this->vertex_array_id);
  glDrawArrays(GL_TRIANGLES, 0, 3);
}
