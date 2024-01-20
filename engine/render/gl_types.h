#pragma once

#include <cstdint>

struct Dimensions {
  uint32_t width;
  uint32_t height;
};

enum class ShaderType { Vertex = 0, Fragment };
