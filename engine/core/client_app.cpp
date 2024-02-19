#include "client_app.h"

#include "GLFW/glfw3.h"
#include "io/logging.h"
#include "render/window.h"
#include "util/err.h"

ClientApp::ClientApp(u32 server_port, u32 client_port)
    : client(std::make_shared<Net::Client>(server_port, client_port)) {}

Err ClientApp::init() {
  Err err = this->window.init({1280, 720});
  if (err.is_error) {
    return err;
  }

  err = this->context.init(this->window.dimensions);
  if (err.is_error) {
    return err;
  }

  this->window.register_callbacks(this);
  this->client->register_callbacks(this);

  return Err::ok();
}

Err ClientApp::run() {
  this->running = true;

  this->client->begin();
  io::debug("started client");

  double prev_time = glfwGetTime();
  double accumulator = 0.0;
  const double dt = 0.1;

  while (this->running) {
    double now = glfwGetTime();
    double frame_time = now - prev_time;
    prev_time = now;

    accumulator += frame_time;

    this->window.poll_events();

    while (accumulator >= dt) {
      this->network_update();

      accumulator -= dt;
    }

    this->context.clear();
    this->context.render();
    this->window.swap_buffers();
  }

  return Err::ok();
}

void ClientApp::shutdown() {
  this->window.shutdown();
  this->client->shutdown();
}

void ClientApp::on_window_resize(Dimensions dimensions) {
  io::debug("Resizing window");
  this->context.resize(dimensions);
}

void ClientApp::on_window_close() {
  io::debug("Closing window");
  this->running = false;
}

void ClientApp::on_connection_accepted(const Net::Message &message) {
  io::debug("Received ConnectionAccepted");
}

void ClientApp::on_connection_denied(const Net::Message &message) {
  io::debug("Received ConnectionDenied");
}

void ClientApp::on_ping(const Net::Message &message) {
  io::debug("Received Ping");
}

void ClientApp::network_update() { this->client->ping_server(); }
