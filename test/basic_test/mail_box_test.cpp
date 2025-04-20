#include <gtest/gtest.h>
#include <iostream>
#include <span>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "basic/mail_box.h"

namespace XH::TEST {
TEST(MailBoxTest, loopback) {
    XH::mail_box box;
    XH::mail_sender sender;

    int port = 12345; // server port
    std::string ip = "127.0.0.1"; // server ip
    ASSERT_EQ(box.bind(ip, port), 0) << "Failed to bind mail box";

    // Register a handler to process incoming messages
    std::unique_ptr<XH::msg_buf> msg_buf = nullptr;
    box.regist_handler([&msg_buf](XH::mail_box* o, std::unique_ptr<XH::msg_buf>&& msg) {
        msg_buf = std::move(msg);
        o->remove_event();
    });

    // create a event base
    struct event_base* base = event_base_new();
    ASSERT_NE(base, nullptr) << "Failed to create event base";
    box.add_event(base);
    std::string msg = "hello";
    sender.send(ip, port, std::span<uint8_t>(reinterpret_cast<uint8_t*>(msg.data()), msg.size()));
    event_base_dispatch(base);
    ASSERT_NE(msg_buf, nullptr) << "Failed to receive message";
    ASSERT_EQ(msg_buf->buf.size(), 5) << "Received message size mismatch";
    ASSERT_EQ(std::memcmp(msg_buf->buf.data(), "hello", 5), 0) << "Received message content mismatch";
    // Clean up
    event_base_free(base);
}
}