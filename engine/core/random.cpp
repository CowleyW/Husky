#include "random.h"

#include <random>

Random::Random() : rng(std::random_device()()) {}

Random::Random(uint64_t seed) : rng(seed) {}

uint32_t Random::random_u32() {
  return std::uniform_int_distribution<uint32_t>()(this->rng);
}

uint32_t Random::random_u32(uint32_t max) {
  return std::uniform_int_distribution<uint32_t>(0, max)(this->rng);
}

uint64_t Random::random_u64() {
  return std::uniform_int_distribution<uint64_t>()(this->rng);
}

uint64_t Random::random_u64(uint64_t max) {
  return std::uniform_int_distribution<uint64_t>(0, max)(this->rng);
}