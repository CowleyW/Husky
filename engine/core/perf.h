#pragma once

#include "io/logging.h"

#include <chrono>

#define PERF_BEGIN(name)                                                       \
  auto _macro_perf_begin_##name = std::chrono::steady_clock::now()

#define PERF_END(name)                                                         \
  do {                                                                         \
    auto _macro_perf_end_##name = std::chrono::steady_clock::now();            \
    auto _macro_perf_elapsed =                                                 \
        _macro_perf_end_##name - _macro_perf_begin_##name;                     \
    float dt_us = std::chrono::duration_cast<std::chrono::microseconds>(       \
                      _macro_perf_elapsed)                                     \
                      .count();                                                \
    io::perf("{} took {}us", #name, dt_us);                                    \
  } while (0)
