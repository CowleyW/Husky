#include "engine/core/application.h"
#include "engine/io/logging.h"

#include <cstring>
#include <string>

void print_usage() {
  io::error("Expected usage:");
  io::error("runtime.exe <client | server> <port>");
}

int main(int argc, char **argv) {
  if (argc != 3) {
    print_usage();
    return 1;
  }

  int port;
  try {
    port = std::stoi(argv[2]);
  } catch (std::exception &e) {
    io::error("Could not parse port {}", argv[2]);
    print_usage();
    return 1;
  }

  bool is_server;
  if (std::strcmp(argv[1], "client") == 0) {
    is_server = false;
  } else if (std::strcmp(argv[1], "server") == 0) {
    is_server = true;
  } else {
    print_usage();
    return 1;
  }

  Result<Application *> result = Application::create(is_server, port);
  if (result.is_error) {
    io::fatal(result.msg);
    return 1;
  }

  Application *app = result.value;

  io::debug("Running application.");
  Err err = app->run();
  if (err.is_error) {
    io::fatal(err.msg);
    return 1;
  }

  io::debug("Shutting down application.");
  app->shutdown();

  delete app;
  return 0;
}
