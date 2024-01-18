#include "application.h"
#include "util/err.h"

Err Application::init() {
  Err err = this->window.init();
  if (!err.isOk) {
    return err;
  }

  return Err::ok();
}

Err Application::run() {
  this->window.loop();
  return Err::ok();
}

void Application::shutdown() {
  this->window.shutdown();
}
