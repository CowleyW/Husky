#pragma once

#include "application.h"
#include "net/server.h"
#include "types.h"
#include "util/err.h"
#include <memory>

class ServerApp : public Application {
public:
  ServerApp(u32 port);

public:
  Err run() override;
  void shutdown() override;

private:
  std::unique_ptr<Net::Server> server;
  bool running = false;
};
