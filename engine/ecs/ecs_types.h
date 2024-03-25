#pragma once

#include <cstdint>

#define MAX_ENTITIES 1000

#define MAX_COMPONENTS 64

#define COMPONENT_DECLARATION(x)                                               \
private:                                                                       \
  static constexpr uint32_t COMPONENT_ID = x;                                  \
                                                                               \
public:                                                                        \
  static uint32_t id() {                                                       \
    return COMPONENT_ID;                                                       \
  }
