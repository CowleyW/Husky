#include "application.h"

#include "core/client_app.h"
#include "core/server_app.h"

Result<Application *> Application::create(bool is_server, u32 port) {
  if (!is_server) {
    return Result<Application *>::ok(new ClientApp(port));
  } else {
    return Result<Application *>::ok(new ServerApp(port));
  }
}
