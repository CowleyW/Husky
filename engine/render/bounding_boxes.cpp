#include "bounding_boxes.h"
#include "glm/fwd.hpp"
#include <limits>

AABB::AABB(const std::vector<Vertex> &vertices) : min(), max() {
  float big = std::numeric_limits<float>::max();
  this->min = glm::vec4(big, big, big, 0);
  this->max = glm::vec4(-big, -big, -big, 0);

  for (const Vertex &v : vertices) {
    this->min.x = std::min(v.position.x, this->min.x);
    this->min.y = std::min(v.position.y, this->min.y);
    this->min.z = std::min(v.position.z, this->min.z);

    this->max.x = std::max(v.position.x, this->max.x);
    this->max.y = std::max(v.position.y, this->max.y);
    this->max.z = std::max(v.position.z, this->max.z);
  }
}
