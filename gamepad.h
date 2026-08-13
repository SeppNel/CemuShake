#pragma once
#include "config.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <atomic>
#include <thread>
#include <vector>

class Gamepad {
  public:
    explicit Gamepad(std::vector<ConfiguredButton> buttons);
    void Start();
    void Stop();
    std::vector<ConfiguredButton> &GetButtonStates();
    bool IsAutomaticShakeActive() const;
    void HandleControllerDisconnected(SDL_Event const &event); // called from main thread when controller is disconnected
  private:
    std::vector<ConfiguredButton> configButtons_;
    std::atomic<bool> stopFlag_{false};
    std::atomic<bool> automaticShake_{false};
    std::unique_ptr<std::thread> thread_;
    std::atomic<SDL_GameController *> controller_{nullptr};

    void run();
    void processAutoShake();
};