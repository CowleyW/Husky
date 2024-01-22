#include "engine/core/application.h"
#include "engine/io/logging.h"

int main(int argc, char **argv) {
  io::debug("Beginning application startup.");

  Application app;
  Err err = app.init();
  if (!err.isOk) {
    io::fatal(err.msg);
    return -1;
  }

  io::debug("Running application.");
  err = app.run();
  if (!err.isOk) {
    io::fatal(err.msg);
    return -1;
  }

  io::debug("Beginning application shutdown.");
  app.shutdown();
  return 0;
}
