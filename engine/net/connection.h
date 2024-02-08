#pragma once

#include "message.h"

#include "core/types.h"
#include "util/err.h"

#include <asio.hpp>

namespace Net {

class Connection {
public:
  Connection();

  void free();
  Err bind(u32 remote_id);

  bool is_connected();
  bool matches_id(u32 id);

  void write_message(const Message &message);

private:
  bool connected;
  // We establish random remote ids since IP Adresses can be spoofed
  u32 remote_id;

  u32 sequence_id;
  u32 ack;
  u32 ack_bitfield;

  u32 message_id;
};

} // namespace Net
