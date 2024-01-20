#pragma once

#include "render/gl_types.h"
#include "render/shader.h"
#include "util/err.h"

#include <cstdint>

class Context {
public:
  Err init(Dimensions dimensions);
  void resize(Dimensions dimension);
  void clear();
  void render();

private:
  uint32_t vbuf_id, ibuf_id;
  uint32_t vertex_array_id;
  Shader shader;
};
