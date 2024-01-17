#include "io/logging.h"
#include "core/window.h"

int main(int argc, char **argv) {
  logging::console_debug("Beginning application startup.");

  Window window;
  window.init();
  window.loop();

  logging::console_debug("Beginning application shutdown.");
  return 0;
}
