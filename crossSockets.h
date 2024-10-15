#include <iostream>

#ifdef __unix__

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

#endif

#ifdef _WIN32

#include <WinSock2.h>

#pragma comment(lib, "Ws2_32.lib")

#define ssize_t int
#define socklen_t int
#define INET6_ADDRSTRLEN 46
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12) // Workaround windows udp bug https://stackoverflow.com/questions/34242622/windows-udp-sockets-recvfrom-fails-with-error-10054

#endif

namespace crossSockets {

const char *GetIP(sockaddr_in const &addr, char *buf);
ssize_t SendPacket(int const &socketFd, std::pair<uint16_t, void const *> const &outBuf, sockaddr_in const &sockInClient);
void setSocketOptionsTimeout(int socketFd, int secs);
void initializeSockets();
void setSocketToNonBlocking(int &socketFd);

} // namespace crossSockets