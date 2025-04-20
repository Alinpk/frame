#include "basic/mail_box.h"
#include <arpa/inet.h>
#include <cassert>

namespace XH {
mail_box::mail_box() noexcept
{}
mail_box::~mail_box() noexcept
{
    if (m_mail_event)
    {
        event_free(m_mail_event);
    }
    if (m_fd >= 0)
    {

        close(m_fd);
    }
}

int mail_box::bind(const std::string& ip, int port) noexcept
{
    m_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (m_fd < 0)
    {
        return -1;
    }

    evutil_make_socket_nonblocking(m_fd);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if (::bind(m_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        return -1;
    }

    return 0;
}

void mail_box::regist_handler(HandlerT&& handler) noexcept
{
    m_handler = std::move(handler);
}

void mail_box::onRead(evutil_socket_t fd, short events, void* arg) noexcept
{
    mail_box* o = reinterpret_cast<mail_box*>(arg);
    assert(o != nullptr);

    auto msg = std::make_unique<msg_buf>();

    msg->buf.resize(MAX_MSG_SIZE);
    socklen_t addr_len = sizeof(msg->src_addr);

    int received_bytes = recvfrom(fd, reinterpret_cast<void*>(msg->buf.data()), msg->buf.size() - 1, 0, (struct sockaddr*)&(msg->src_addr), &addr_len);
    if (received_bytes < 0)
    {
        LOG_WARN("recv from socket failed: %s", strerror(errno));
        return;
    }

    msg->buf.resize(received_bytes);

    // Call the user-registered handler
    o->m_handler(o, std::move(msg));
}

void mail_box::add_event(struct event_base* base) noexcept
{
    m_mail_event = event_new(base, m_fd, EV_READ | EV_PERSIST, onRead, this);
    if (m_mail_event)
    {
        event_add(m_mail_event, nullptr);
    }
    else
    {
        LOG_ERROR("Failed to create event");
    }
    m_event_base = base;
}

void mail_box::remove_event() noexcept
{
    if (m_mail_event)
    {
        event_del(m_mail_event);
        event_free(m_mail_event);
        m_mail_event = nullptr;
        m_event_base = nullptr;
    }
}

mail_sender::~mail_sender() noexcept
{
    if (m_fd >= 0)
    {
        close(m_fd);
    }
}

mail_sender::mail_sender() noexcept
{}

void mail_sender::set_dst(const std::string& ip, int port) noexcept
{
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
    m_addr.sin_port = htons(port);
    if (m_fd < 0)
    {
        m_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (m_fd < 0)
        {
            LOG_ERROR("Failed to create socket: %s", strerror(errno));
            return;
        }
        evutil_make_socket_nonblocking(m_fd);
    }
}

int mail_sender::send(const std::string& ip, int port, std::span<uint8_t> data) noexcept
{
    set_dst(ip, port);
    return sendto(m_fd, data.data(), data.size(), 0, (struct sockaddr*)&m_addr, sizeof(m_addr));
}
} // namespace XH