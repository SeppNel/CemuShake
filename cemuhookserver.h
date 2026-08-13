#include "cemuhookprotocol.h"
#include "config.h"
#include "crossSockets.h"
#include "gamepad.h"
#include <SDL2/SDL_gamecontroller.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

using namespace cemuhook_protocol;

class Server {
  public:
    explicit Server(const Config *const cfg, Gamepad *gamepad);
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

    struct Gyro_Compensation_Data {
        // Every non-zero motion sample is recorded here while it happens,
        // so it can be replayed in reverse once motion stops.
        std::vector<std::array<float, 3>> history;

        void reset() {
            history.clear();
        }
    };

    uint32_t serverPort = 26760;
    const bool gyro_compensation;
    Gamepad *gamepad = nullptr;
    bool stopFlag = false;
    int socketFd;
    std::unique_ptr<std::thread> sendThread;
    std::unique_ptr<std::thread> runThread;
    SharedResponse sharedResponse;
    VersionInformation versionAnswer;
    InfoAnswer infoAnswer;
    InfoAnswer infoNoneAnswer;
    DataEvent dataAnswer;
    std::vector<Client> clients;
    Gyro_Compensation_Data gyro_tracker;

    void run();
    void sendTask();
    void PrepareAnswerConstants();
    void CalcCrcDataAnswer();
    void handleClientsTimeout();
    std::pair<uint16_t, void const *> PrepareInfoAnswer(uint8_t const &slot);
    std::pair<uint16_t, void const *> PrepareDataAnswer(uint32_t const &packet);
    void proccess_gyro_compensation();
};
