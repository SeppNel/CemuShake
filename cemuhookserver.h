#include "cemuhookprotocol.h"
#include "config.h"
#include "crossSockets.h"
#include "gamepad.h"

#include <cstddef>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <vector>

using namespace cemuhook_protocol;

class Server {
  public:
    explicit Server(uint32_t port, Gamepad *gamepad);
    void Start();
    void Stop();

  private:
    struct Client {
        sockaddr_in address;
        uint32_t id;
        int sendTimeout;

        bool operator==(sockaddr_in const &other);
        bool operator!=(sockaddr_in const &other);
    };

    uint32_t serverPort = 26760;
    Gamepad *gamepad = nullptr;

    bool stopFlag = false;

    int socketFd;

    std::unique_ptr<std::thread> sendThread;
    std::unique_ptr<std::thread> runThread;

    void run();
    void sendTask();

    SharedResponse sharedResponse;
    VersionInformation versionAnswer;
    InfoAnswer infoAnswer;
    InfoAnswer infoNoneAnswer;
    DataEvent dataAnswer;

    void PrepareAnswerConstants();

    // std::pair<uint16_t, void const *> PrepareVersionAnswer(uint32_t const &id);
    std::pair<uint16_t, void const *> PrepareInfoAnswer(uint8_t const &slot);
    std::pair<uint16_t, void const *> PrepareDataAnswer(uint32_t const &packet);
    void CalcCrcDataAnswer();

    std::vector<Client> clients;
    void handleClientsTimeout();
};
