#include "crossSockets.h"

namespace crossSockets {

const char *GetIP(sockaddr_in const &addr, char *buf) {
#ifdef __unix__
    return inet_ntop(addr.sin_family, &(addr.sin_addr.s_addr), buf, INET6_ADDRSTRLEN);
#endif
#ifdef _WIN32
    // TODO
    return "TODO";
#endif
}

ssize_t SendPacket(int const &socketFd, std::pair<uint16_t, void const *> const &outBuf, sockaddr_in const &sockInClient) {
#ifdef __unix__
    void const *buffer = outBuf.second;
#endif
#ifdef _WIN32
    const char *buffer = (const char *)outBuf.second;
#endif

    return sendto(socketFd, buffer, outBuf.first, 0, (sockaddr *)&sockInClient, sizeof(sockInClient));
}

void setSocketOptionsTimeout(int socketFd, int secs) {
#ifdef __unix__
    timeval read_timeout;
    read_timeout.tv_sec = secs;
    read_timeout.tv_usec = 0;
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
#endif
#ifdef _WIN32
    DWORD timeout = secs * 1000; // Milliseconds
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
#endif
}

void initializeSockets() {
#ifdef _WIN32
    WSADATA wsaData;

    // Initialize Winsock (Windows-specific).
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Server: Winsock initialization failed. Error: " << WSAGetLastError() << std::endl;
        return;
    }
#endif

    return;
}

} // namespace crossSockets