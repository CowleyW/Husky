#pragma once

#include <random>

class Random {
public:
  Random();
  Random(uint64_t seed);

  uint32_t random_u32();
  uint32_t random_u32(uint32_t max);

  uint64_t random_u64();
  uint64_t random_u64(uint64_t max);

private:
  std::mt19937 rng;
};
