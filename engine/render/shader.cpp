#include "shader.h"

#include "io/files.h"
#include "render/gl_types.h"
#include "util/err.h"

#include <glad/gl.h>

#include <stdint.h>
#include <string_view>

Err Shader::init(std::string_view vertex_path, std::string_view fragment_path) {
  // 1. Compile the vertex shader
  Result<uint32_t> vertex_result =
      Shader::compile_shader(ShaderType::Vertex, vertex_path);
  if (vertex_result.is_error) {
    return Err::err(vertex_result.msg);
  }
  uint32_t vshader_id = vertex_result.value;

  // 2. Compile the fragment shader
  Result<uint32_t> fragment_result =
      Shader::compile_shader(ShaderType::Fragment, fragment_path);
  if (fragment_result.is_error) {
    glDeleteShader(vshader_id);
    return Err::err(fragment_result.msg);
  }
  uint32_t fshader_id = fragment_result.value;

  // 3. Link the shaders
  this->program_id = glCreateProgram();

  glAttachShader(this->program_id, vshader_id);
  glAttachShader(this->program_id, fshader_id);
  glLinkProgram(this->program_id);

  int32_t link_result;
  glGetProgramiv(this->program_id, GL_LINK_STATUS, &link_result);
  if (!link_result) {
    glDeleteShader(vshader_id);
    glDeleteShader(fshader_id);
    return Err::err("Failed to link shaders");
  }

  // 4. Cleanup
  glDeleteShader(vshader_id);
  glDeleteShader(fshader_id);

  return Err::ok();
}

Result<uint32_t> Shader::compile_shader(ShaderType type,
                                        std::string_view path) {
  auto source_result = files::load_text_file(std::string(path));
  if (source_result.is_error) {
    return Result<uint32_t>::err(source_result.msg);
  }
  std::string shader_source = source_result.value;

  GLenum shader_type =
      (type == ShaderType::Vertex) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
  uint32_t shader_id = glCreateShader(shader_type);

  // glShaderSource(...) expects a const char ** so we first assign
  // the source to a local variable before passing a pointer to it.
  const char *shader_cstr = shader_source.c_str();
  glShaderSource(shader_id, 1, &shader_cstr, nullptr);
  glCompileShader(shader_id);

  int32_t result;
  glGetShaderiv(shader_id, GL_COMPILE_STATUS, &result);
  if (!result) {
    return Result<uint32_t>::err("Failed to compile shader");
  }

  return Result<uint32_t>::ok(shader_id);
}

void Shader::bind() { glUseProgram(this->program_id); }
