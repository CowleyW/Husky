#pragma once

#include "ecs_types.h"

#include <bitset>
#include <cstdint>

struct Entity {
  uint64_t id;
  std::bitset<MAX_COMPONENTS> mask;
};
