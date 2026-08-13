#include "cemuhookserver.h"
#include "config.h"
#include "gamepad.h"
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <sys/types.h>
#include <yaml-cpp/yaml.h>

using std::cout;
using namespace std::chrono;

#define SLEEP_TIME_MS 100

bool stopFlag = false;

void signalHandler(int signal) {
    if (signal == SIGINT) {
        cout << "\nCtrl + C Received, Stopping...\n";
        stopFlag = true;
    }
}

Config *readConfig() {
    Config *configStruct = new Config();

    std::string configPath = "./CemuShake.yml";

    if (!std::filesystem::exists(configPath)) {
#ifdef _WIN32
        const char *homeDir = getenv("USERPROFILE");
#else
        const char *homeDir = getenv("HOME");
#endif
        if (homeDir != nullptr) {
            configPath = std::string(homeDir) + "/.config/CemuShake.yml";
        }
    }

    try {
        YAML::Node configFile = YAML::LoadFile(configPath);

        configStruct->gyro_compensation = configFile["gyro_compensation"].as<bool>();
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

    // Initialize SDL
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        cout << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    Config *configStruct = readConfig();
    if (configStruct->buttons.size() == 0) {
        configStruct->buttons.emplace_back(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, false, 0.0f, 200.0f, 0.0f, 0.0f, 0.0f, 0.0f); // Default: RB = Shake up, no gyro;
    }

    Gamepad gamepad(configStruct->buttons);
    gamepad.Start();
    Server server(configStruct, &gamepad);
    server.Start();

    SDL_Event event;
    while (!stopFlag) {
        while (SDL_PollEvent(&event)) {
            // Check for quit events
            if (event.type == SDL_QUIT) {
                stopFlag = true;
                break;
            } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
                gamepad.HandleControllerDisconnected(event);
            }
        }

        std::this_thread::sleep_for(milliseconds(SLEEP_TIME_MS));
    }

    server.Stop();
    gamepad.Stop();
    SDL_Quit();
    return 0;
}