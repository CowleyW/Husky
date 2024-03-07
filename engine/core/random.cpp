#include "random.h"

#include "types.h"

#include <random>

Random::Random() : rng(std::random_device()()) {}

Random::Random(u64 seed) : rng(seed) {}

u32 Random::random_u32() {
  return std::uniform_int_distribution<u32>()(this->rng);
}

u32 Random::random_u32(u32 max) {
  return std::uniform_int_distribution<u32>(0, max)(this->rng);
}

u64 Random::random_u64() {
  return std::uniform_int_distribution<u64>()(this->rng);
}

u64 Random::random_u64(u64 max) {
  return std::uniform_int_distribution<u64>(0, max)(this->rng);
}