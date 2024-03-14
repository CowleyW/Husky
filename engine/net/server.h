#pragma once

#include "asio/io_context.hpp"
#include "core/types.h"

#include "client_slot.h"
#include "listener.h"
#include "net/message_handler.h"
#include "core/world_state.h"

#include <asio.hpp>

#include <thread>
#include <vector>

namespace Net {

class Server : MessageHandler {
public:
  Server(u32 port, u8 max_clients);

  void begin();
  void shutdown();

  std::vector<ClientSlot> &get_clients();

  std::optional<u8> next_new_client();
  std::optional<u8> next_disconnected_client();

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
  get_by_client_salt(u64 client_salt, const asio::ip::udp::endpoint &remote);

  std::optional<ClientSlot *const>
  get_by_xor_salt(u64 xor_salt, const asio::ip::udp::endpoint &remote);
  std::optional<ClientSlot *const> get_by_xor_salt(u64 xor_salt);

  std::optional<ClientSlot *const>
  get_by_salts(u64 client_salt, u64 server_salt,
               const asio::ip::udp::endpoint &remote);

  std::optional<ClientSlot *const> get_open();

  void accept(const asio::ip::udp::endpoint &remote, u64 client_salt);
  void challenge(const asio::ip::udp::endpoint &remote, u64 client_salt);
  void deny_connection(const asio::ip::udp::endpoint &remote);

  bool has_open_slot();

private:
  u32 port;

  u8 max_clients;
  u8 num_connected_clients;
  std::vector<ClientSlot> clients;

  std::queue<u8> new_clients;
  std::queue<u8> disconnected_clients;

  std::unique_ptr<asio::io_context> context;
  // asio::ip::udp::socket socket;
  Listener listener;
  Sender denier;
  asio::ip::udp::endpoint remote;

  std::thread context_thread;
  std::vector<u8> recv_buf;
};

} // namespace Net
