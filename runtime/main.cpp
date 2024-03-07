#include "engine/core/application.h"
#include "engine/io/logging.h"

#include <cstring>
#include <string>

void print_usage() {
  io::error("Expected usage:");
  io::error("runtime.exe <client | server> <port>");
}

int main(int argc, char **argv) {
  if (argc < 3) {
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

  Result<Application *> result;
  if (is_server) {
    int server_port = std::stoi(argv[2]);
    result = Application::create_server(server_port);
  } else {
    int server_port = std::stoi(argv[2]);
    int client_port = std::stoi(argv[3]);
    result = Application::create_client(server_port, client_port);
  }

  if (result.is_error) {
    io::fatal(result.msg);
    return 1;
  }

  Application *app = result.value;

  io::debug("Running application.");
  app->run();

  delete app;
  return 0;
}
