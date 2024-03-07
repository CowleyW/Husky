#pragma once

#include "types.h"

#include <random>

class Random {
public:
  Random();
  Random(u64 seed);

  u32 random_u32();
  u32 random_u32(u32 max);

  u64 random_u64();
  u64 random_u64(u64 max);

private:
  std::mt19937 rng;
};