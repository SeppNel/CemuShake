#include <SDL2/SDL_gamecontroller.h>
#include <cstdint>
#include <vector>

#pragma once

typedef unsigned int uint;

struct ConfiguredButton {
    SDL_GameControllerButton button;
    bool pending;
    float accX;
    float accY;
    float accZ;
    float pitch;
    float yaw;
    float roll;

    ConfiguredButton(SDL_GameControllerButton btn, bool pend, float aX, float aY, float aZ, float p, float y, float r) {
        button = btn;
        pending = pend;
        accX = aX;
        accY = aY;
        accZ = aZ;
        pitch = p;
        yaw = y;
        roll = r;
    }

    ConfiguredButton(int btn, bool pend, float aX, float aY, float aZ, float p, float y, float r) {
        button = (SDL_GameControllerButton)btn;
        pending = pend;
        accX = aX;
        accY = aY;
        accZ = aZ;
        pitch = p;
        yaw = y;
        roll = r;
    }
};

struct Config {
    uint port = 26760;
    std::vector<ConfiguredButton> buttons;
};
