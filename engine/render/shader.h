#pragma once

#include "util/err.h"
#include "util/result.h"

#include "render/gl_types.h"

#include <cstdint>

class Shader {
public:
  Err init(std::string_view vertex_path, std::string_view fragment_path);
  void bind();
  void cleanup();

private:
  static Result<uint32_t> compile_shader(ShaderType type,
                                         std::string_view path);

private:
  uint32_t program_id;
};
