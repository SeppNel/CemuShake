#include "cemuhookserver.h"
#include <csignal>

Server *serverPointer;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        serverPointer->Stop();  // Stop the loop on Ctrl+C
    }
}
int main(){
    std::signal(SIGINT, signalHandler);
    Server server;
}