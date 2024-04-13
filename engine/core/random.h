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

  float random_float();
  float random_float(float max);
  float random_float(float min, float max);

private:
  std::mt19937 rng;
};
