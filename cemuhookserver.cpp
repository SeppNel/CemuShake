#include "cemuhookserver.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>

using std::cout;
using namespace std::chrono;

#define PORT 26760
#define BUFLEN 100
#define SERVER_ID 69
#define MAIN_SLEEP_TIME_M 500
#define THREAD_SLEEP_TIME_U 50
#define GYRO_STR 20

#define VERSION_TYPE 0x100000
#define INFO_TYPE 0x100001
#define DATA_TYPE 0x100002

const char *GetIP(sockaddr_in const &addr, char *buf) {
    return inet_ntop(addr.sin_family, &(addr.sin_addr.s_addr), buf, INET6_ADDRSTRLEN);
}

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

ssize_t SendPacket(int const &socketFd, std::pair<uint16_t, void const *> const &outBuf, sockaddr_in const &sockInClient) {
    return sendto(socketFd, outBuf.second, outBuf.first, 0, (sockaddr *)&sockInClient, sizeof(sockInClient));
}

Server::Server() {
    PrepareAnswerConstants();
    Start();
}

Server::~Server() {}

void Server::Stop() {
    stopSending = true;
    stopServer = true;
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

void Server::Start() {
    cout << "Server: Initializing.\n";

    socketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    timeval read_timeout;
    read_timeout.tv_sec = 2;
    read_timeout.tv_usec = 0;
    setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));

    if (socketFd == -1)
        throw std::runtime_error("Server: Socket could not be created.");

    sockaddr_in sockAddr;

    sockAddr = sockaddr_in();

    sockAddr.sin_family = AF_INET;
    if (const char *customPort = std::getenv("CEMUSHAKE_SERVER_PORT")) {
        sockAddr.sin_port = htons(std::atoi(customPort));
    } else {
        sockAddr.sin_port = htons(PORT);
    }
    sockAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socketFd, (sockaddr *)&sockAddr, sizeof(sockAddr)) < 0)
        throw std::runtime_error("Server: Bind failed.");

    char ipStr[INET6_ADDRSTRLEN];
    ipStr[0] = 0;
    // cout << "Server: Socket created at IP: " << GetIP(sockAddr,ipStr) << " Port: " << ntohs(sockAddr.sin_port) << ".";

    char buf[BUFLEN];
    sockaddr_in sockInClient;
    socklen_t sockInClientLen = sizeof(sockInClient);

    ssize_t headerSize = (ssize_t)sizeof(Header);

    std::pair<uint16_t, void const *> outBuf;

    std::unique_ptr<std::thread> sendThread;
    inputThread.reset(new std::thread(&Server::inputTask, this));

    // cout << "Server: Start listening for client.\n";

    while (!stopServer) {
        auto recvLen = recvfrom(socketFd, buf, BUFLEN, 0, (sockaddr *)&sockInClient, &sockInClientLen);
        if (recvLen >= headerSize) {
            Header &header = *reinterpret_cast<Header *>(buf);

            std::ostringstream addressTextStream;
            addressTextStream << "IP: " << GetIP(sockInClient, ipStr) << " Port: " << ntohs(sockInClient.sin_port);
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
                    SendPacket(socketFd, outBuf, sockInClient);
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

                    if (sendThread.get() == nullptr) {
                        stopSending = false;
                        sendThread.reset(new std::thread(&Server::sendTask, this));
                    }
                } else {
                    // cout << "Server: Request for data from existing client. " << addressText << ".\n";
                    client->sendTimeout = 0;
                }
                break;
            }
        }

        std::this_thread::sleep_for(milliseconds(MAIN_SLEEP_TIME_M));
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

    while (!stopSending) {
        outBuf = PrepareDataAnswer(++packet);
        for (auto &client : clients) {
            SendPacket(socketFd, outBuf, client.address);
        }
        std::this_thread::sleep_for(microseconds(THREAD_SLEEP_TIME_U));
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

    if (inputPendingUp) {
        if (dataAnswer.motion.accY == 200) {
            dataAnswer.motion.accY = 0;
            dataAnswer.motion.pitch = 0;
        } else {
            dataAnswer.motion.accY = 200;
            dataAnswer.motion.pitch = -GYRO_STR;
        }
        inputPendingUp = false;
    } else if (inputPendingDown) {
        if (dataAnswer.motion.accY == 200) {
            dataAnswer.motion.accY = 0;
            dataAnswer.motion.pitch = 0;
        } else {
            dataAnswer.motion.accY = 200;
            dataAnswer.motion.pitch = GYRO_STR;
        }
        inputPendingDown = false;
    } else {
        dataAnswer.motion.accX = 0;
        dataAnswer.motion.accY = 0;
        dataAnswer.motion.accZ = 0;
    }

    CalcCrcDataAnswer();

    return std::pair<uint16_t, void const *>(len, reinterpret_cast<void *>(&dataAnswer));
}

void Server::CalcCrcDataAnswer() {
    static const uint16_t len = sizeof(dataAnswer);

    dataAnswer.header.crc32 = 0;
    dataAnswer.header.crc32 = crc32(reinterpret_cast<unsigned char *>(&dataAnswer), len);
}

SDL_GameController *findController() {
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

void Server::inputTask() {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return;
    }

    SDL_Event event;
    SDL_GameController *controller = findController();

    while (!stopServer) {
        while (SDL_PollEvent(&event)) {
            // Check for quit events
            if (event.type == SDL_QUIT) {
                SDL_Quit();
                return;
            }
        }

        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_LEFTSHOULDER)) {
            inputPendingUp = true;
        }

        if (SDL_GameControllerGetButton(controller, SDL_GameControllerButton::SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) {
            inputPendingDown = true;
        }

        std::this_thread::sleep_for(microseconds(THREAD_SLEEP_TIME_U));
    }

    SDL_Quit();
}

bool Server::Client::operator==(sockaddr_in const &other) {
    return address.sin_addr.s_addr == other.sin_addr.s_addr && address.sin_port == other.sin_port;
}

bool Server::Client::operator!=(sockaddr_in const &other) {
    return !(*this == other);
}
