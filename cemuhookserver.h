#include "cemuhookprotocol.h"
#include <thread>
#include <netinet/in.h>
#include <mutex>
#include <vector>
#include <shared_mutex>

using namespace cemuhook::protocol;

class Server
{
    public:
    Server();
    void Stop();

    ~Server();

    private:

    struct Client
    {
        sockaddr_in address;
        uint32_t id;
        int sendTimeout;

        bool operator==(sockaddr_in const& other);
        bool operator!=(sockaddr_in const& other);
    };

    bool stopServer = false;
    bool stopSending;
    bool inputPendingUp = false;
    bool inputPendingDown = false;

    int socketFd;

    std::unique_ptr<std::thread> inputThread;

    void serverTask();
    void sendTask();
    void Start();
    void inputTask();

    SharedResponse sharedResponse;
    VersionInformation versionAnswer;
    InfoAnswer infoAnswer;
    InfoAnswer infoNoneAnswer;
    DataEvent dataAnswer;

    void PrepareAnswerConstants();

    std::pair<uint16_t , void const*> PrepareVersionAnswer(uint32_t const& id);
    std::pair<uint16_t , void const*> PrepareInfoAnswer(uint8_t const& slot);
    std::pair<uint16_t , void const*> PrepareDataAnswer(uint32_t const& packet);
    void ModifyDataAnswerId(uint32_t const& id);
    void CalcCrcDataAnswer();

    std::vector<Client> clients;
};

