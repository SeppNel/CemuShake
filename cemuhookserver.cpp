#include "cemuhookserver.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <vector>

using std::cout;
using namespace std::chrono;

#define BUFLEN 50
#define SERVER_ID 69
#define MAIN_SLEEP_TIME_MS 500
#define THREAD_SLEEP_TIME_MS 5
#define CLIENT_TIMEOUT 40 // 20 Seconds ((MAIN_SLEEP_TIME_MS + socketTimeout) * CLIENT_TIMEOUT / 1000)

#define VERSION_TYPE 0x100000
#define INFO_TYPE 0x100001
#define DATA_TYPE 0x100002

namespace {

uint32_t crc32(const unsigned char *s, size_t n) {
    uint32_t crc = 0xFFFFFFFF;

    int k;

    while (n--) {
        crc ^= *s++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
    }
    return ~crc;
}

bool motion_is_zero(DataEvent &dataAnswer) {
    return dataAnswer.motion.accX == 0.0F &&
           dataAnswer.motion.accY == 0.0F &&
           dataAnswer.motion.accZ == 0.0F &&
           dataAnswer.motion.pitch == 0.0F &&
           dataAnswer.motion.yaw == 0.0F &&
           dataAnswer.motion.roll == 0.0F;
}

} // namespace

Server::Server(const Config *cfg, Gamepad *g)
    : serverPort(cfg->port),
      gyro_compensation(cfg->gyro_compensation),
      gamepad(g) {
    PrepareAnswerConstants();
}

void Server::Start() {
    stopFlag = false;
    runThread.reset(new std::thread(&Server::run, this));
    sendThread.reset(new std::thread(&Server::sendTask, this));
}

void Server::Stop() {
    stopFlag = true;
    if (sendThread.get() != nullptr) {
        sendThread->join();
    }
    if (runThread.get() != nullptr) {
        runThread->join();
    }
}

void Server::PrepareAnswerConstants() {
    cout << "Server: Pre-filling messages.\n";
    Header outHeader;
    outHeader.magic[0] = 'D';
    outHeader.magic[1] = 'S';
    outHeader.magic[2] = 'U';
    outHeader.magic[3] = 'S';
    outHeader.version = 1001;
    outHeader.id = SERVER_ID;

    versionAnswer.header = outHeader;
    versionAnswer.header.length = sizeof(versionAnswer.version) + 4;
    versionAnswer.version = 1001;

    sharedResponse.slot = 0;
    sharedResponse.slotState = 2;
    sharedResponse.deviceModel = 2;
    sharedResponse.connection = 1;
    sharedResponse.mac1 = 0;
    sharedResponse.mac2 = 0;
    sharedResponse.battery = 0;

    infoAnswer.header = outHeader;
    infoAnswer.header.eventType = INFO_TYPE;
    infoAnswer.header.length = sizeof(sharedResponse) + sizeof(infoAnswer.zero) + 4;
    infoAnswer.response = sharedResponse;
    infoAnswer.zero = 0;

    infoNoneAnswer.header = outHeader;
    infoNoneAnswer.header.eventType = INFO_TYPE;
    infoNoneAnswer.header.length = sizeof(sharedResponse) + sizeof(infoNoneAnswer.zero) + 4;
    infoNoneAnswer.response = sharedResponse;
    infoNoneAnswer.response.slotState = 0;
    infoNoneAnswer.response.deviceModel = 0;
    infoNoneAnswer.response.connection = 0;
    infoNoneAnswer.zero = 0;

    dataAnswer.header = outHeader;
    dataAnswer.header.eventType = DATA_TYPE;
    dataAnswer.header.length = sizeof(dataAnswer) - sizeof(dataAnswer.header) + 4;
    dataAnswer.response = sharedResponse;
    dataAnswer.connected = 1;

    char *dataAnswerPointer = reinterpret_cast<char *>(&dataAnswer.buttons1);
    uint8_t len = 32; // From buttons1 to touch (32 bytes)
    for (int i = 0; i < len; i++) {
        // clear most data
        dataAnswerPointer[i] = 0;
    }

    dataAnswer.motion.accX = 0;
    dataAnswer.motion.accY = 0;
    dataAnswer.motion.accZ = 0;

    dataAnswer.motion.pitch = 0;
    dataAnswer.motion.yaw = 0;
    dataAnswer.motion.roll = 0;
}

void Server::run() {
    cout << "Server: Initializing.\n";

    crossSockets::initializeSockets();

    socketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    crossSockets::setSocketOptionsTimeout(socketFd, 2);
    crossSockets::setSocketToNonBlocking(socketFd);

    // Workaround windows udp bug https://stackoverflow.com/questions/34242622/windows-udp-sockets-recvfrom-fails-with-error-10054
#ifdef _WIN32
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(socketFd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
#endif

    if (socketFd == -1)
        throw std::runtime_error("Server: Socket could not be created.");

    sockaddr_in sockAddr;
    sockAddr = sockaddr_in();

    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(serverPort);
    sockAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketFd, (sockaddr *)&sockAddr, sizeof(sockAddr)) < 0)
        throw std::runtime_error("Server: Bind failed.");

    char ipStr[INET6_ADDRSTRLEN];
    ipStr[0] = 0;
    cout << "Server: Socket created at IP: " << crossSockets::GetIP(sockAddr, ipStr) << " Port: " << ntohs(sockAddr.sin_port) << ".\n";

    char buf[BUFLEN];
    sockaddr_in sockInClient;
    socklen_t sockInClientLen = sizeof(sockInClient);
    ssize_t headerSize = (ssize_t)sizeof(Header);
    std::pair<uint16_t, void const *> outBuf;

    cout << "Server: Start listening for client.\n";

    while (!stopFlag) {
        auto recvLen = recvfrom(socketFd, buf, BUFLEN, 0, (sockaddr *)&sockInClient, &sockInClientLen);
        if (recvLen >= headerSize) {
            Header &header = *reinterpret_cast<Header *>(buf);

            std::ostringstream addressTextStream;
            addressTextStream << "IP: " << crossSockets::GetIP(sockInClient, ipStr) << " Port: " << ntohs(sockInClient.sin_port);
            auto addressText = addressTextStream.str();

            switch (header.eventType) {
            case VERSION_TYPE:
                // cout << "Server: A client asked for version. " << addressText << ".\n";
                break;
            case INFO_TYPE: {
                // cout << "Server: A client asked for controller info. " << addressText << ".\n";
                InfoRequest &req = *reinterpret_cast<InfoRequest *>(buf + headerSize);
                for (int i = 0; i < req.portCnt; i++) {
                    outBuf = PrepareInfoAnswer(req.slots[i]);
                    crossSockets::SendPacket(socketFd, outBuf, sockInClient);
                }
            } break;
            case DATA_TYPE:
                auto client = std::find(clients.begin(), clients.end(), sockInClient);
                if (client == clients.end()) {
                    // cout << "Server: Request for data from new client. " << addressText << ".\n";

                    Client &newClient = clients.emplace_back();
                    newClient.address = sockInClient;
                    newClient.id = header.id;
                    newClient.sendTimeout = 0;

                    cout << "Server: New client subscribed. " << addressText << ".\n";
                } else {
                    // cout << "Server: Request for data from existing client. " << addressText << ".\n";
                    client->sendTimeout = 0;
                }
                break;
            }
        }

        handleClientsTimeout();

        std::this_thread::sleep_for(milliseconds(MAIN_SLEEP_TIME_MS));
    }
}

void Server::handleClientsTimeout() {
    for (auto it = clients.begin(); it != clients.end();) {
        it->sendTimeout++;
        if (it->sendTimeout >= CLIENT_TIMEOUT) {
            it = clients.erase(it);
            cout << "Client timed out\n";
        } else {
            ++it;
        }
    }
}

std::pair<uint16_t, void const *> Server::PrepareInfoAnswer(uint8_t const &slot) {
    static const uint16_t len = sizeof(infoNoneAnswer);

    if (slot != 0) {
        infoNoneAnswer.response.slot = slot;
        infoNoneAnswer.header.crc32 = 0;
        infoNoneAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&infoNoneAnswer), len);
        return std::pair<uint16_t, void const *>(len, reinterpret_cast<void *>(&infoNoneAnswer));
    }

    infoAnswer.header.crc32 = 0;
    infoAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&infoAnswer), len);
    return std::pair<uint16_t, void const *>(len, reinterpret_cast<void *>(&infoAnswer));
}

void Server::sendTask() {
    std::pair<uint16_t, void const *> outBuf;
    uint32_t packet = 0;

    while (!stopFlag) {
        outBuf = PrepareDataAnswer(++packet);
        for (auto &client : clients) {
            crossSockets::SendPacket(socketFd, outBuf, client.address);
        }
        std::this_thread::sleep_for(milliseconds(THREAD_SLEEP_TIME_MS));
    }
}

std::pair<uint16_t, void const *> Server::PrepareDataAnswer(uint32_t const &packet) {
    static const uint16_t len = sizeof(dataAnswer);
    dataAnswer.packetNumber = packet;

    high_resolution_clock::duration tp = high_resolution_clock::now().time_since_epoch();
    microseconds us = duration_cast<microseconds>(tp);

    dataAnswer.motion.timestamp = us.count();

    dataAnswer.motion.accX = 0;
    dataAnswer.motion.accY = 0;
    dataAnswer.motion.accZ = 0;

    dataAnswer.motion.pitch = 0;
    dataAnswer.motion.yaw = 0;
    dataAnswer.motion.roll = 0;

    static uint16_t automatic_cnt = 0;
    if (gamepad->IsAutomaticShakeActive()) {
        if (automatic_cnt % 2 == 0) {
            dataAnswer.motion.accX = 500;
        }

        automatic_cnt++;
    } else {
        automatic_cnt = 0;
    }

    std::vector<ConfiguredButton> &configButtons = gamepad->GetButtonStates();

    for (size_t i = 0; i < configButtons.size(); i++) {
        if (configButtons[i].pending) {
            dataAnswer.motion.accX = configButtons[i].accX;
            dataAnswer.motion.accY = configButtons[i].accY;
            dataAnswer.motion.accZ = configButtons[i].accZ;
            dataAnswer.motion.pitch = configButtons[i].pitch;
            dataAnswer.motion.yaw = configButtons[i].yaw;
            dataAnswer.motion.roll = configButtons[i].roll;
            configButtons[i].pending = false;
        }
    }

    if (gyro_compensation)
        proccess_gyro_compensation();

    CalcCrcDataAnswer();

    return std::pair<uint16_t, void const *>(len, reinterpret_cast<void *>(&dataAnswer));
}

void Server::CalcCrcDataAnswer() {
    static const uint16_t len = sizeof(dataAnswer);

    dataAnswer.header.crc32 = 0;
    dataAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&dataAnswer), len);
}

void Server::proccess_gyro_compensation() {
    if (!motion_is_zero(dataAnswer)) {
        gyro_tracker.history.push_back({dataAnswer.motion.pitch, dataAnswer.motion.yaw, dataAnswer.motion.roll});
    } else if (!gyro_tracker.history.empty()) {
        auto const &sample = gyro_tracker.history.back();
        dataAnswer.motion.pitch = -sample[0];
        dataAnswer.motion.yaw = -sample[1];
        dataAnswer.motion.roll = -sample[2];
        gyro_tracker.history.pop_back();
    }
}

bool Server::Client::operator==(sockaddr_in const &other) {
    return address.sin_addr.s_addr == other.sin_addr.s_addr && address.sin_port == other.sin_port;
}

bool Server::Client::operator!=(sockaddr_in const &other) {
    return !(*this == other);
}
