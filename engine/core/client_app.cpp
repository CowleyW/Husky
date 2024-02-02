#include "client_app.h"

#include "io/logging.h"
#include "render/window.h"
#include "util/err.h"

ClientApp::ClientApp(u32 port) : client(std::make_shared<Net::Client>(port)) {}

Err ClientApp::init() {
  Err err = this->window.init({1280, 720});
  if (!err.isOk) {
    return err;
  }

  err = this->context.init(this->window.dimensions);
  if (!err.isOk) {
    return err;
  }

  this->window.register_callbacks(this);

  return Err::ok();
}

Err ClientApp::run() {
  this->running = true;

  while (this->running) {
    this->window.poll_events();

    this->context.clear();
    this->context.render();
    this->window.swap_buffers();
  }

  return Err::ok();
}

void ClientApp::shutdown() { this->window.shutdown(); }

void ClientApp::on_window_resize(Dimensions dimensions) {
  io::debug("Resizing window");
  this->context.resize(dimensions);
}

void ClientApp::on_window_close() {
  io::debug("Closing window");
  this->running = false;
}
