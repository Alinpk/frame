#pragma once

#include "basic/log.h"
#include <cstring>
#include "event2/event.h"
#include "event2/util.h"
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <span>
#include <functional>

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
    using HandlerT = std::function<void(mail_box*, std::unique_ptr<msg_buf>&&)>;

    mail_box() noexcept;

    ~mail_box() noexcept;

    // Listen on the specified IP and port
    int bind(const std::string& ip, int port) noexcept;

    // Register a handler to process incoming messages
    void regist_handler(HandlerT&& handler) noexcept;

    // Get the libevent socket for external use
    evutil_socket_t get_sock() const noexcept { return m_fd; }

    // Add a new event to the event loop
    void add_event(struct event_base* base) noexcept;

    // Remove the event from the event loop
    void remove_event() noexcept;

private:
    static void onRead(evutil_socket_t fd, short events, void* arg) noexcept;

    evutil_socket_t m_fd{-1};
    struct event* m_mail_event{nullptr};
    struct event_base* m_event_base{nullptr};
    HandlerT m_handler = HandlerT{};
    static constexpr int MAX_MSG_SIZE = 1024; // Maximum message size
};

class mail_sender
{
public:
    mail_sender() noexcept;
    ~mail_sender() noexcept;

    // Send a message to the specified IP and port
    int send(const std::string& ip, int port, std::span<uint8_t> data) noexcept;
private:
    void set_dst(const std::string& ip, int port) noexcept;

    int m_fd{-1};
    struct sockaddr_in m_addr{};
};
} // namespace XH
