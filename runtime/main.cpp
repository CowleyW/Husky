#include "core/application.h"
#include "engine/io/logging.h"
#include "engine/core/window.h"

int main(int argc, char **argv) {
  logging::console_debug("Beginning application startup.");

  Application app;
  Err err = app.init();
  if (!err.isOk) {
    logging::console_fatal(err.msg);
    return -1;
  }

  logging::console_debug("Running application.");
  err = app.run();
  if (!err.isOk) {
    logging::console_fatal(err.msg);
    return -1;
  }

  logging::console_debug("Beginning application shutdown.");
  app.shutdown();
  return 0;
}
