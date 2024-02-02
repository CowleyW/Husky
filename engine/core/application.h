#pragma once

#include "types.h"
#include "util/err.h"
#include "util/result.h"

class Application {
public:
  virtual Err run() = 0;
  virtual void shutdown() = 0;

  virtual ~Application() {}

  static Result<Application *> create(bool is_server, u32 port);
};
