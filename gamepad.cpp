#include "gamepad.h"
#include <iostream>

#define CONTROLLER_WAIT_MS 1000
#define THREAD_SLEEP_TIME_MS 5
#define AUTO_SHAKE_DUR_MS 4000
constexpr uint16_t AUTO_SHAKE_DUR = AUTO_SHAKE_DUR_MS / THREAD_SLEEP_TIME_MS;

using std::cout;
using namespace std::chrono;

SDL_GameController *findController() {
    SDL_JoystickUpdate();
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            return SDL_GameControllerOpen(i);
        }
    }

    return nullptr;
}

Gamepad::Gamepad(std::vector<ConfiguredButton> buttons)
    : configButtons_(std::move(buttons)) {
    controller_ = findController();
}

void Gamepad::Start() {
    stopFlag_ = false;
    thread_.reset(new std::thread(&Gamepad::run, this));
}

void Gamepad::Stop() {
    stopFlag_ = true;
    if (thread_.get() != nullptr) {
        thread_->join();
    }
}

std::vector<ConfiguredButton> &Gamepad::GetButtonStates() {
    return configButtons_;
}

bool Gamepad::IsAutomaticShakeActive() const {
    return automaticShake_;
}

void Gamepad::processAutoShake() {
    if (!automaticShake_ && SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_BACK) &&
        SDL_GameControllerGetButton(controller_, SDL_CONTROLLER_BUTTON_START)) {
        cout << "Automatic shake started\n";
        automaticShake_ = true;
    }

    static uint16_t automatic_cnt = 0;
    if (automaticShake_) {
        automatic_cnt++;

        if (automatic_cnt == AUTO_SHAKE_DUR) {
            cout << "Automatic shake ended\n";
            automaticShake_ = false;
            automatic_cnt = 0;
        }
    }
}

void Gamepad::run() {
    while (!stopFlag_) {
        if (controller_ == nullptr) {
            controller_ = findController();
            while (controller_ == nullptr && !stopFlag_) {
                controller_ = findController();
                std::this_thread::sleep_for(milliseconds(CONTROLLER_WAIT_MS));
            }
            if (!stopFlag_)
                cout << "Controller reconnected\n";
        }

        SDL_GameControllerUpdate();

        processAutoShake();

        for (size_t i = 0; i < configButtons_.size(); i++) {
            if (SDL_GameControllerGetButton(controller_, configButtons_[i].button)) {
                configButtons_[i].pending = true;
            }
        }

        std::this_thread::sleep_for(milliseconds(THREAD_SLEEP_TIME_MS));
    }
}

void Gamepad::HandleControllerDisconnected(SDL_Event const &event) {
    if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        cout << "Controller disconnected\n";
        if (controller_ && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller_))) {
            SDL_GameControllerClose(controller_);
            controller_ = nullptr;
        }
    }
}