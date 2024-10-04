#include "cemuhookserver.h"
#include <csignal>
#include <iostream>

using std::cout;

Server *serverPointer;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\nCtrl + C Received, Stopping...\n";
        serverPointer->Stop();
    }
}
int main() {
    std::signal(SIGINT, signalHandler);
    Server server;
    serverPointer = &server;
    server.Start();
}