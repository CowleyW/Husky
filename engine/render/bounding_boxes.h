#pragma once

#include "glm/glm.hpp"
#include "render/vertex.h"
#include <vector>

struct AABB {
  // We ignore the w values in both of these vectors, they are simply there to
  // make the padding explicit
  glm::vec4 min;
  glm::vec4 max;

  AABB(const std::vector<Vertex> &vertices);
};
