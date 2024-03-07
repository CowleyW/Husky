#pragma once

#include "types.h"
#include "util/err.h"
#include "util/result.h"

class Application {
public:
  void run();
  void stop();

  virtual void begin() {}
  virtual void update() {}
  virtual void fixed_update() {}
  virtual void render() {}
  virtual void shutdown() {}

  virtual ~Application() {}

  static Result<Application *> create_server(u32 port);
  static Result<Application *> create_client(u32 server_port, u32 client_port);

private:
  bool running;
};
