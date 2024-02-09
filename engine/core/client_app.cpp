#include "client_app.h"

#include "io/logging.h"
#include "render/window.h"
#include "util/err.h"

ClientApp::ClientApp(u32 port) : client(std::make_shared<Net::Client>(port)) {}

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

  return Err::ok();
}

Err ClientApp::run() {
  this->running = true;

  this->client->begin();
  io::debug("started client");

  while (this->running) {
    this->window.poll_events();

    // if (this->client->has_message()) {
    //   Net::Message message = this->client->get_message();
    //
    //   std::string mbody = reinterpret_cast<char *>(message.body.data());
    //   io::info("message: {}", mbody);
    // }

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
