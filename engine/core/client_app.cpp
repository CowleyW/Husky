#include "client_app.h"

#include "ecs/components.h"
#include "io/input_map.h"
#include "io/logging.h"
#include "io/raw_inputs.h"
#include "random.h"
#include "render/material.h"
#include "util/err.h"
#include "util/serialize.h"
#include "world_state.h"

#include "GLFW/glfw3.h"
#include "glm/fwd.hpp"
#include "imgui.h"

#include <cmath>
#include <tracy/Tracy.hpp>

ClientApp::ClientApp(uint32_t server_port, uint32_t client_port)
    : client(std::make_shared<Net::Client>(server_port, client_port)),
      frame(0),
      world_state(),
      registry(),
      inputs(),
      render_engine({1920, 1080}, this) {
}

Err ClientApp::init() {
  ZoneScopedN("ClientApp::init");

  TriMeshHandle golem = TriMesh::get("models/mech_golem.asset").value;
  TriMeshHandle dwarf = TriMesh::get("models/fort_golem.asset").value;
  MaterialHandle mat1 =
      Material::get(TriMesh::get_texture_name(golem).value).value;
  MaterialHandle mat2 =
      Material::get(TriMesh::get_texture_name(dwarf).value).value;
  this->render_engine.upload_mesh(golem);
  this->render_engine.upload_mesh(dwarf);
  this->render_engine.upload_material(Material::get(mat1).value);
  this->render_engine.upload_material(Material::get(mat2).value);
  Mesh dwarf_mesh = {dwarf, mat2, true};
  Mesh golem_mesh = {golem, mat1, true};
  Transform transform(
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, 1.0f});
  Camera camera = {{0.0f, 0.0f, 1.0f}, 70.0f, 0.1f, 200.0f, 0.0f, 0.0f};
  const auto cam = this->registry.create();
  this->registry.emplace<Transform>(cam, transform);
  this->registry.emplace<Camera>(cam, camera);

  Random rand;
  for (uint32_t i = 0; i < 5000; i += 1) {
    const auto e = this->registry.create();
    float r = 75.0 * std::sqrt(rand.random_float(1.0));
    float theta = 2 * 3.1415926 * rand.random_float(1.0);
    float rot = 2 * 3.1415926 * rand.random_float(1.0);
    float scale = rand.random_float(0.5f, 1.0f);

    float x = r * std::cos(theta);
    float z = r * std::sin(theta);

    Transform transform({x, 0.0f, z}, {0.0f, rot, 0.0f}, {scale, scale, scale});

    if (i % 2 == 0) {
      this->registry.emplace<Transform>(e, transform);
      this->registry.emplace<Mesh>(e, dwarf_mesh);
    } else {
      this->registry.emplace<Transform>(e, transform);
      this->registry.emplace<Mesh>(e, golem_mesh);
    }
  }
  // for (int x = -6; x <= 6; x += 3) {
  //   for (int z = -6; z <= 6; z += 3) {
  //     Transform t({x, 0.0f, z}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f});
  //     const auto e = this->registry.create();
  //     this->registry.emplace<Transform>(e, t);
  //     this->registry.emplace<Mesh>(e, golem_mesh);
  //   }
  // }

  auto group = this->registry.group<Mesh, Transform>();
  group.sort<Mesh, Transform>([](const auto &lhs, const auto &rhs) {
    uint32_t lhs_mat = (uint32_t)std::get<0>(lhs).material;
    uint32_t rhs_mat = (uint32_t)std::get<0>(rhs).material;
    uint32_t rhs_mesh = (uint32_t)std::get<0>(lhs).mesh;
    uint32_t lhs_mesh = (uint32_t)std::get<0>(rhs).mesh;
    if (lhs_mat != rhs_mat) {
      return lhs_mat < rhs_mat;
    } else {
      return lhs_mesh < rhs_mesh;
    }
  });

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
      float yaw = (float)this->inputs.mouse_dx * 0.16f;
      float pitch = (float)this->inputs.mouse_dy * 0.16f;

      c.rotate(yaw, pitch);
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
