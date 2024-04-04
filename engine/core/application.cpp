#include "application.h"

#include "core/client_app.h"
#include "core/server_app.h"
#include "io/logging.h"

#include <chrono>

Result<Application *> Application::create_server(uint32_t port) {
  return Result<Application *>::ok(new ServerApp(port));
}

Result<Application *>
Application::create_client(uint32_t server_port, uint32_t client_port) {
  ClientApp *app = new ClientApp(server_port, client_port);
  Err err = app->init();
  if (err.is_error) {
    delete app;

    return Result<Application *>::err(err.msg);
  }
  return Result<Application *>::ok(app);
}

void Application::run() {
  // The entirety of the application life cycle happens inside this method. We
  // call begin() to set up whatever is necessary to run
  this->begin();
  this->running = true;

  // Now is time to set up the core game loop.
  // https://gafferongames.com/post/fix_your_timestep/
  using namespace std::literals::chrono_literals;
  constexpr std::chrono::nanoseconds dt(16ms);

  std::chrono::nanoseconds accumulator(0ns);
  auto prev_time = std::chrono::steady_clock::now();

  while (this->running) {
    auto now = std::chrono::steady_clock::now();
    auto frame_time = now - prev_time;
    float dt_ms =
        std::chrono::duration_cast<std::chrono::microseconds>(frame_time)
            .count() /
        1000.0f;
    prev_time = now;

    accumulator += frame_time;

    // Perform any updates that should happen as regularly as possible
    this->update(dt_ms);

    // Perform fixed update as many times as needed
    while (accumulator >= dt) {
      accumulator -= dt;

      this->fixed_update();
    }

    // Render (if there is anything to render)
    this->render();
  }

  this->shutdown();
}

void Application::stop() {
  this->running = false;
}
