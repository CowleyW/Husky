#include "application.h"

#include "core/client_app.h"
#include "core/server_app.h"

Result<Application *> Application::create_server(u32 port) {
  return Result<Application *>::ok(new ServerApp(port));
}

Result<Application *> Application::create_client(u32 server_port,
                                                 u32 client_port) {
  ClientApp *app = new ClientApp(server_port, client_port);
  Err err = app->init();
  if (err.is_error) {
    delete app;

    return Result<Application *>::err(err.msg);
  }
  return Result<Application *>::ok(app);
}
