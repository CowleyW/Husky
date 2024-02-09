#include "application.h"

#include "core/client_app.h"
#include "core/server_app.h"

Result<Application *> Application::create(bool is_server, u32 port) {
  if (!is_server) {
    ClientApp *app = new ClientApp(port);
    Err err = app->init();
    if (err.is_error) {
      delete app;

      return Result<Application *>::err(err.msg);
    }
    return Result<Application *>::ok(app);
  } else {
    return Result<Application *>::ok(new ServerApp(port));
  }
}
