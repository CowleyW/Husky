#pragma once

#include "asio/io_context.hpp"

#include "client_slot.h"
#include "core/world_state.h"
#include "listener.h"
#include "net/message_handler.h"

#include <asio.hpp>

#include <thread>
#include <vector>

namespace Net {

class Server : MessageHandler {
public:
  Server(uint32_t port, uint8_t max_clients);

  void begin();
  void shutdown();

  std::vector<ClientSlot> &get_clients();

  std::optional<uint8_t> next_new_client();
  std::optional<uint8_t> next_disconnected_client();

  void ping_all();
  void send_world_state(const WorldState &world_state);

public:
  void on_connection_requested(const Net::Message &message,
                               const asio::ip::udp::endpoint &remote) override;

  void on_challenge_response(const Net::Message &message,
                             const asio::ip::udp::endpoint &remote) override;

  void on_disconnected(const Net::Message &message,
                       const asio::ip::udp::endpoint &remote) override;

  void on_ping(const Net::Message &message,
               const asio::ip::udp::endpoint &remote) override;

  void on_user_inputs(const Net::Message &message,
                      const asio::ip::udp::endpoint &remote) override;

private:
  std::optional<ClientSlot *const>
  get_by_endpoint(const asio::ip::udp::endpoint &remote);

  std::optional<ClientSlot *const>
  get_by_client_salt(uint64_t client_salt,
                     const asio::ip::udp::endpoint &remote);

  std::optional<ClientSlot *const>
  get_by_xor_salt(uint64_t xor_salt, const asio::ip::udp::endpoint &remote);
  std::optional<ClientSlot *const> get_by_xor_salt(uint64_t xor_salt);

  std::optional<ClientSlot *const>
  get_by_salts(uint64_t client_salt, uint64_t server_salt,
               const asio::ip::udp::endpoint &remote);

  std::optional<ClientSlot *const> get_open();

  void accept(const asio::ip::udp::endpoint &remote, uint64_t client_salt);
  void challenge(const asio::ip::udp::endpoint &remote, uint64_t client_salt);
  void deny_connection(const asio::ip::udp::endpoint &remote);

  bool has_open_slot();

private:
  uint32_t port;

  uint8_t max_clients;
  uint8_t num_connected_clients;
  std::vector<ClientSlot> clients;

  std::queue<uint8_t> new_clients;
  std::queue<uint8_t> disconnected_clients;

  std::unique_ptr<asio::io_context> context;
  Listener listener;
  Sender denier;
  asio::ip::udp::endpoint remote;

  std::thread context_thread;
  std::vector<uint8_t> recv_buf;
};

} // namespace Net
