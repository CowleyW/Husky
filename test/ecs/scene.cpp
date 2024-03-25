#include "engine/ecs/scene.h"
#include "engine/ecs/components.h"

#include <bitset>
#include <catch2/catch_test_macros.hpp>

#include <vector>

template <typename... Components>
std::bitset<MAX_COMPONENTS> test_works() {
  std::bitset<MAX_COMPONENTS> component_mask = {};
  uint32_t ids[] = {Components::id()...};
  for (uint32_t i = 0; i < (sizeof...(Components)); i += 1) {
    component_mask.set(ids[i]);
  }

  return component_mask;
}

TEST_CASE("wtf", "[WTF]") {
  std::bitset<MAX_COMPONENTS> actual = test_works<Transform, Mesh>();

  std::bitset<MAX_COMPONENTS> expected;
  expected.set(0);
  expected.set(1);

  REQUIRE(actual == expected);

  actual = test_works<Mesh>();
  expected.reset(0);

  REQUIRE(actual == expected);

  actual = test_works<Transform>();
  expected.set(0);
  expected.reset(1);

  REQUIRE(actual == expected);
}
