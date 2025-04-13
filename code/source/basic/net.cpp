#include "basic/net.h"
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace XH {
std::optional<std::string> getLocalIPWithBoost() noexcept
{
    std::string localIP;

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return std::nullopt;
    }

    char hostname[NI_MAXHOST];
    if (gethostname(hostname, NI_MAXHOST) != 0) {
        std::cerr << "gethostname failed." << std::endl;
        WSACleanup();
        return std::nullopt;
    }

    addrinfo hints, *res, *p;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
        std::cerr << "getaddrinfo failed." << std::endl;
        WSACleanup();
        return std::nullopt;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        sockaddr_in* ipv4 = (sockaddr_in*)p->ai_addr;
        if (ipv4->sin_addr.s_addr != INADDR_LOOPBACK) {
            char ipstr[INET_ADDRSTRLEN];
            inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
            localIP = ipstr;
            break;
        }
    }

    freeaddrinfo(res);
    WSACleanup();
#else
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "getifaddrs failed." << std::endl;
        return std::nullopt;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            if (ntohl(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr) != INADDR_LOOPBACK) {
                char ipstr[INET_ADDRSTRLEN];
                inet_ntop(ifa->ifa_addr->sa_family, addr, ipstr, sizeof(ipstr));
                localIP = ipstr;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
#endif

    return localIP;
}
} // namespace XH
