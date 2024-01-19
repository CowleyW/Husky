#pragma once

#include "render/gl_types.h"
#include "util/err.h"
#include <cstdint>

class Context {
public:
  Err init(Dimensions dimensions);
  void resize(Dimensions dimension);
  void clear();
  void render();

private:
  uint32_t vbuf_id;
  uint32_t vshader_id, fshader_id, program_id;
  uint32_t vertex_array_id;
};
