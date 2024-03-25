#include "client_app.h"

#include "io/input_map.h"
#include "io/logging.h"
#include "util/err.h"
#include "util/serialize.h"
#include "world_state.h"

ClientApp::ClientApp(uint32_t server_port, uint32_t client_port)
    : client(std::make_shared<Net::Client>(server_port, client_port)),
      frame(0),
      world_state(),
      scene() {
  this->mesh = TriMesh::load_from_obj("objs/mech_golem.obj").value;
  Mesh mesh_comp = {&mesh, true};
  Transform transform = {
      {0.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 0.0f},
      {1.0f, 1.0f, 1.0f}};
  uint64_t e1 = this->scene.new_entity();
  this->scene.assign(e1, mesh_comp);
  this->scene.assign(e1, transform);

  transform.position.y = 1.0f;

  uint64_t e2 = this->scene.new_entity();
  this->scene.assign(e2, mesh_comp);
  this->scene.assign(e2, transform);

  transform.position.y = -1.0f;

  uint64_t e3 = this->scene.new_entity();
  this->scene.assign(e3, mesh_comp);
  this->scene.assign(e3, transform);
}

Err ClientApp::init() {
  Err err = this->render_engine.init({1280, 720}, this);
  if (err.is_error) {
    return err;
  }

  return Err::ok();
}

void ClientApp::begin() {
  this->client->begin();
}

void ClientApp::update() {
  this->render_engine.poll_events();
  this->poll_network();
}

void ClientApp::render() {
  this->render_engine.render(this->scene);
}

void ClientApp::fixed_update() {
  this->frame += 1;

  InputMap inputs = this->render_engine.get_inputs();

  if (this->client_index.has_value()) {
    auto pos = this->world_state.player_position(this->client_index.value());

    if (pos.is_error) {
      io::error("{} {}", pos.msg, this->client_index.value());
    } else {
      io::debug("client position: [{}, {}]", pos.value.x, pos.value.y);
    }
  }

  if (this->frame % 6 == 0) {
    this->network_update(inputs);
    this->frame = 0;
  }
}

void ClientApp::shutdown() {
  this->client->shutdown();
}

void ClientApp::on_window_resize(Dimensions dimensions) {
  io::debug("Resizing window");
  this->render_engine.resize(dimensions);
}

void ClientApp::on_window_close() {
  io::debug("Closing window");
  this->stop();
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
