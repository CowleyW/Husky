#pragma once

#include "util/err.h"
#include "util/result.h"

class Application {
public:
  void run();
  void stop();

  virtual void begin() {
  }
  virtual void update() {
  }
  virtual void fixed_update() {
  }
  virtual void render() {
  }
  virtual void shutdown() {
  }

  virtual ~Application() {
  }

  static Result<Application *> create_server(uint32_t port);
  static Result<Application *>
  create_client(uint32_t server_port, uint32_t client_port);

private:
  bool running;
};
