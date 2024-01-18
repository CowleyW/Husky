#include "application.h"
#include "io/logging.h"
#include "util/err.h"

Err Application::init() {
  Err err = this->window.init({ 1280, 720 });
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
  this->window.loop();
  return Err::ok();
}

void Application::shutdown() {
  this->window.shutdown();
}

void Application::on_window_resize(Dimensions dimensions) {
  logging::console_debug("Resizing window");
  this->context.resize(dimensions);
}
