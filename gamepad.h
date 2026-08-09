#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include "config.h"

class Gamepad {
public:
    explicit Gamepad(std::vector<ConfiguredButton> buttons);
    void Start();
    void Stop();
    std::vector<ConfiguredButton> GetButtonStates() const; // returns a copy, mutex-guarded
    bool IsAutomaticShakeActive() const;                   // atomic<bool>
    void HandleControllerDisconnected(SDL_Event const &event); // called from main thread when controller is disconnected
private:
    void run();
    void processAutoShake();
    mutable std::mutex mutex_;
    std::vector<ConfiguredButton> configButtons_;
    std::atomic<bool> stopFlag_{false};
    std::atomic<bool> automaticShake_{false};
    std::unique_ptr<std::thread> thread_;
    std::atomic<SDL_GameController *> controller_{nullptr};
};