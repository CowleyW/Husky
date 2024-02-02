#include "connection.h"

#include "message.h"
#include "message_queue.h"

Net::Connection::Connection(asio::io_context &context)
    : socket(context), queue(socket) {}

void write_message(Net::Message &message);

void begin_queue();

bool has_message();
Net::Message get_message();
