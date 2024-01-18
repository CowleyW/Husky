#pragma once

#include "render/gl_types.h"
#include "util/err.h"
#include <cstdint>

class Context {
public:
  Err init(Dimensions dimensions);
  void resize(Dimensions dimension);
};
