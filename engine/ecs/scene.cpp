#include "scene.h"

#include "ecs_types.h"
#include <bitset>
#include <stdexcept>

Scene::Scene() : entities(), components(MAX_COMPONENTS) {
}

uint64_t Scene::new_entity() {
  if (this->entities.size() == MAX_ENTITIES) {
    throw std::overflow_error("Can't add any more entities.");
  }

  uint64_t id = this->entities.size();
  entities.push_back({id, {}});

  return id;
}
