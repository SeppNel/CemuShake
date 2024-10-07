#include "cemuhookprotocol.h"
#include "config.h"
#include <cstddef>
#include <mutex>
#include <netinet/in.h>
#include <shared_mutex>
#include <thread>
#include <vector>

using namespace cemuhook::protocol;

class Server {
  public:
    Server();
    Server(Config *c);
    void Start();
    void Stop();

    ~Server();

  private:
    struct Client {
        sockaddr_in address;
        uint32_t id;
        int sendTimeout;

        bool operator==(sockaddr_in const &other);
        bool operator!=(sockaddr_in const &other);
    };

    Config *configStruct = nullptr;
    uint serverPort = 26760;
    std::vector<ConfiguredButton> configButtons;

    bool stopServer = false;
    bool stopSending = false;

    int socketFd;

    std::unique_ptr<std::thread> sendThread;
    std::unique_ptr<std::thread> inputThread;

    void serverTask();
    void sendTask();
    void inputTask();

    SharedResponse sharedResponse;
    VersionInformation versionAnswer;
    InfoAnswer infoAnswer;
    InfoAnswer infoNoneAnswer;
    DataEvent dataAnswer;

    void PrepareAnswerConstants();

    std::pair<uint16_t, void const *> PrepareVersionAnswer(uint32_t const &id);
    std::pair<uint16_t, void const *> PrepareInfoAnswer(uint8_t const &slot);
    std::pair<uint16_t, void const *> PrepareDataAnswer(uint32_t const &packet);
    void ModifyDataAnswerId(uint32_t const &id);
    void CalcCrcDataAnswer();

    std::vector<Client> clients;
};
