#include "client_app.h"

#include "ecs/components.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "io/raw_inputs.h"
#include "render/material.h"
#include "util/err.h"
#include "util/serialize.h"
#include "world_state.h"

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "imgui.h"

#include <algorithm>
#include <tracy/Tracy.hpp>

ClientApp::ClientApp(uint32_t server_port, uint32_t client_port)
    : client(std::make_shared<Net::Client>(server_port, client_port)),
      frame(0),
      world_state(),
      registry(),
      inputs() {
}

Err ClientApp::init() {
  ZoneScopedN("ClientApp::init");
  Err err = this->render_engine.init({1920, 1080}, this);
  if (err.is_error) {
    return err;
  }

  TriMeshHandle golem = TriMesh::get("models/mech_golem.asset").value;
  TriMeshHandle dwarf = TriMesh::get("models/fort_golem.asset").value;
  MaterialHandle mat1 =
      Material::get("models/FantasyRivals_Texture_01_A.asset").value;
  MaterialHandle mat2 =
      Material::get("models/FantasyRivals_Texture_03_A.asset").value;
  this->render_engine.upload_mesh(TriMesh::get(golem).value);
  this->render_engine.upload_mesh(TriMesh::get(dwarf).value);
  this->render_engine.upload_material(Material::get(mat1).value);
  this->render_engine.upload_material(Material::get(mat2).value);
  Mesh dwarf_mesh = {dwarf, mat1, true};
  Mesh golem_mesh = {golem, mat2, true};
  Transform transform = {
      {35.0f, 2.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, 1.0f}};
  Camera camera = {{0.0f, 0.0f, 1.0f}, 70.0f, 0.1f, 200.0f, 0.0f, 0.0f};
  const auto cam = this->registry.create();
  this->registry.emplace<Transform>(cam, transform);
  this->registry.emplace<Camera>(cam, camera);

  for (uint32_t x = 0; x < 70; x += 1) {
    for (uint32_t z = 0; z < 70; z += 1) {
      const auto e = this->registry.create();
      if ((x + z) % 2 == 0) {
        Transform dwarf_transform = {
            {x, 0.0f, z},
            {0.0f, 3.1415f, 0.0f},
            {0.5f, 0.5f, 0.5f}};
        this->registry.emplace<Transform>(e, dwarf_transform);
        this->registry.emplace<Mesh>(e, dwarf_mesh);
      } else {
        Transform golem_transform = {
            {x, 0.0f, z},
            {0.0f, 3.1415f, 0.0f},
            {1.0f, 1.0f, 1.0f}};
        this->registry.emplace<Transform>(e, golem_transform);
        this->registry.emplace<Mesh>(e, golem_mesh);
      }
    }
  }

  return Err::ok();
}

void ClientApp::begin() {
  this->client->begin();
}

void ClientApp::update(float dt) {
  ZoneScopedN("ClientApp::update");
  this->render_engine.poll_events();
  this->poll_network();

  this->render_engine.imgui_enqueue([=]() {
    ImGui::Begin(
        "Performance",
        &this->perf_tab_active,
        ImGuiWindowFlags_MenuBar);

    ImGui::Text("Frame Time: %.1f", dt);

    ImGui::End();
  });
}

void ClientApp::render() {
  ZoneScopedN("ClientApp::render");
  this->render_engine.render(this->registry);
}

void ClientApp::fixed_update() {
  ZoneScopedN("ClientApp::fixed_update");
  this->frame += 1;

  this->registry.view<Camera, Transform>().each([this](auto &c, auto &t) {
    if (this->inputs.is_key_down(GLFW_KEY_W)) {
      t.position += c.forward * 0.016f;
    }
    if (this->inputs.is_key_down(GLFW_KEY_A)) {
      t.position -= c.calc_right() * 0.016f;
    }
    if (this->inputs.is_key_down(GLFW_KEY_S)) {
      t.position -= c.forward * 0.016f;
    }
    if (this->inputs.is_key_down(GLFW_KEY_D)) {
      t.position += c.calc_right() * 0.016f;
    }
    if (this->inputs.is_key_down(GLFW_KEY_LEFT_SHIFT)) {
      t.position -= glm::vec3(0.0f, 1.0f, 0.0f) * 0.016f;
    }
    if (this->inputs.is_key_down(GLFW_KEY_SPACE)) {
      t.position += glm::vec3(0.0f, 1.0f, 0.0f) * 0.016f;
    }

    if (this->inputs.is_mouse_button_down(GLFW_MOUSE_BUTTON_LEFT)) {
      c.yaw += (float)this->inputs.mouse_dx * 0.16f;
      c.pitch += (float)this->inputs.mouse_dy * 0.16f;
      c.pitch = std::clamp(c.pitch, -89.0f, 89.0f);

      float yaw = (float)this->inputs.mouse_dx * 0.16f;
      float pitch = (float)this->inputs.mouse_dy * 0.16f;

      glm::mat4 yawRotation =
          glm::rotate(glm::mat4(1.0f), glm::radians(-yaw), {0.0f, 1.0f, 0.0f});
      glm::mat4 pitchRotation =
          glm::rotate(glm::mat4(1.0f), glm::radians(-pitch), c.calc_right());

      glm::mat4 combinedRotation = yawRotation * pitchRotation;

      c.forward = glm::normalize(
          glm::vec3(combinedRotation * glm::vec4(c.forward, 0.0f)));
    }
  });
  if (this->client_index.has_value()) {
    auto pos = this->world_state.player_position(this->client_index.value());

    if (pos.is_error) {
      io::error("{} {}", pos.msg, this->client_index.value());
    } else {
      io::debug("client position: [{}, {}]", pos.value.x, pos.value.y);
    }
  }

  InputMap inputs = this->render_engine.get_inputs();
  if (this->frame % 6 == 0) {
    this->network_update(inputs);
    this->frame = 0;
  }

  // Clear the deltas so that if the mouse moved callback doesn't get triggered
  // we don't retain the old values
  this->inputs.mouse_dx = 0.0f;
  this->inputs.mouse_dy = 0.0f;
}

void ClientApp::shutdown() {
  this->client->shutdown();
}

void ClientApp::on_window_resize(Dimensions dimensions) {
  this->render_engine.resize(dimensions);
}

void ClientApp::on_window_close() {
  io::debug("Closing window");
  this->stop();
}

void ClientApp::on_mouse_move(double x, double y) {
  double dx = x - this->inputs.mouse_x;
  double dy = y - this->inputs.mouse_y;

  this->inputs.mouse_x = x;
  this->inputs.mouse_y = y;
  this->inputs.mouse_dx += dx;
  this->inputs.mouse_dy += dy;
}

void ClientApp::on_key_event(KeyCode key, int32_t action) {
  if (action == GLFW_PRESS) {
    this->inputs.keys.set(key);
  } else if (action == GLFW_RELEASE) {
    this->inputs.keys.reset(key);
  }
}

void ClientApp::on_mouse_button(MouseButton button, int32_t action) {
  if (action == GLFW_PRESS) {
    this->inputs.mouse_buttons.set(button);
  } else if (action == GLFW_RELEASE) {
    this->inputs.mouse_buttons.reset(button);
  }
}

void ClientApp::on_connection_accepted(const Net::Message &message) {
  MutBuf<uint8_t> buf(message.body);
  this->client_index = Serialize::deserialize_u8(buf);
  io::debug("[{}]: Received ConnectionAccepted", this->client_index.value());
}

void ClientApp::on_connection_denied(const Net::Message &message) {
  io::debug("Received ConnectionDenied");
}

void ClientApp::on_ping(const Net::Message &message) {
  io::debug("Received Ping");
}

void ClientApp::on_world_snapshot(const WorldState &world_state) {
  this->world_state = world_state;
  io::debug("Received world state: {} clients", world_state.player_count());
}

void ClientApp::network_update(const InputMap &inputs) {
  if (this->client->is_connected()) {
    this->client->send_inputs(inputs);
  }
}

void ClientApp::poll_network() {
  if (this->client->is_connected() && this->client->maybe_timeout()) {
    io::debug("server timed out");
  }

  while (true) {
    auto maybe = this->client->next_message();

    if (maybe.has_value()) {
      this->handle_message(maybe.value());
    } else {
      break;
    }
  }
}

void ClientApp::handle_message(const Net::Message &message) {
  io::debug(
      "[s_id: {}] [ack: {} | bits: {:b}]",
      message.header.sequence_id,
      message.header.ack,
      message.header.ack_bitfield);
  switch (message.header.message_type) {
  case Net::MessageType::ConnectionAccepted:
    this->on_connection_accepted(message);
    break;
  case Net::MessageType::ConnectionDenied:
    this->on_connection_denied(message);
    break;
  case Net::MessageType::Ping:
    this->on_ping(message);
    break;
  case Net::MessageType::WorldSnapshot: {
    Buf<uint8_t> buf(message.body);
    auto world_state = WorldState::deserialize(buf);
    if (world_state.is_error) {
      io::error("Failed to deserialize WorldState: {}", world_state.msg);
    } else {
      this->on_world_snapshot(world_state.value);
    }
    break;
  }
  default:
    io::error("Unknown message type {}", (uint8_t)message.header.message_type);
    break;
  }
}
