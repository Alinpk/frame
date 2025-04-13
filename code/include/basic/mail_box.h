#pragma once

#include "basic/log.h"
#include <cstring>
#include <event2/event.h>
#include <event2/util.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace XH {

// A wrapper for incoming messages
struct msg_buf
{
    sockaddr_in src_addr;      // Source address of the message
    std::vector<uint8_t> buf; // Message content
};

class mail_box
{
public:
    using Handler = std::function<void(std::unique_ptr<msg_buf>&&)>;

    mail_box() noexcept;

    ~mail_box() noexcept;

    // Listen on the specified IP and port
    int bind(const std::string& ip, int port) noexcept;

    // Register a handler to process incoming messages
    void regist_handler(Handler&& handler) noexcept;

    // Get the libevent socket for external use
    evutil_socket_t get_sock() const noexcept { return m_fd; }

    // Add a new event to the event loop
    void add_event(struct event_base* base) noexcept;

private:
    static void onRead(evutil_socket_t fd, short events, void* arg) noexcept;

    evutil_socket_t m_fd{-1};
    struct event* m_mail_event{nullptr};
    Handler m_handler = [](std::unique_ptr<msg_buf>&&){};
    static constexpr int MAX_MSG_SIZE = 1024; // Maximum message size
};
} // namespace XH
