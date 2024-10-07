#include "cemuhookserver.h"
#include "config.h"
#include <csignal>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <yaml-cpp/yaml.h>

using std::cout;

Server *serverPointer;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\nCtrl + C Received, Stopping...\n";
        serverPointer->Stop();
    }
}

Config *readConfig() {
    Config *configStruct = new Config();

    std::string homeDir = getenv("HOME");
    std::string configPath = homeDir + "/.config/CemuShake.yml";
    try {
        YAML::Node configFile = YAML::LoadFile(configPath);

        configStruct->port = configFile["port"].as<uint>();

        for (std::size_t i = 0; i < configFile["buttons"].size(); i++) {
            configStruct->buttons.emplace_back(
                configFile["buttons"][i]["id"].as<int>(),
                false,
                configFile["buttons"][i]["accX"].as<float>(),
                configFile["buttons"][i]["accY"].as<float>(),
                configFile["buttons"][i]["accZ"].as<float>(),
                configFile["buttons"][i]["pitch"].as<float>(),
                configFile["buttons"][i]["yaw"].as<float>(),
                configFile["buttons"][i]["roll"].as<float>());
        }

    } catch (YAML::BadFile) {
        cout << "Could not load config file\n";
    }

    return configStruct;
}

int main() {
    std::signal(SIGINT, signalHandler);
    Config *configStruct = readConfig();
    Server server(configStruct);
    serverPointer = &server;
    server.Start();
}