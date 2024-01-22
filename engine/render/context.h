#pragma once

#include "core/types.h"
#include "render/gl_types.h"
#include "render/shader.h"
#include "util/err.h"

class Context {
public:
  Err init(Dimensions dimensions);
  void resize(Dimensions dimension);
  void clear();
  void render();

private:
  u32 vbuf_id, ibuf_id;
  u32 vertex_array_id;
  Shader shader;
};
