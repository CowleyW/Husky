#include "application.h"
#include "io/logging.h"
#include "util/err.h"

Err Application::init() {
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

Err Application::run() {
  this->running = true;

  while (this->running) {
    this->window.poll_events();

    this->context.clear();
    this->context.render();
    this->window.swap_buffers();
  }

  return Err::ok();
}

void Application::shutdown() { this->window.shutdown(); }

void Application::on_window_resize(Dimensions dimensions) {
  logging::console_debug("Resizing window");
  this->context.resize(dimensions);
}

void Application::on_window_close() {
  logging::console_debug("Closing window");
  this->running = false;
}
