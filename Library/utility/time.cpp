#include "utility/time.h"

using namespace slim;

Time::Time() {
    start = std::chrono::high_resolution_clock::now();
    clock = std::chrono::high_resolution_clock::now();
}

void Time::Update() {
    clock = std::chrono::high_resolution_clock::now();
}

double Time::Delta() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = now - clock;
    return delta.count() / 1000.0;
}

double Time::Elapsed() const {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> delta = now - start;
    return delta.count() / 1000.0;
}

FPS::FPS() {
    time = std::chrono::system_clock::now();
    value = 0.0;
}

void FPS::Update() {
    frames++;
    auto curr = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = curr - time;
    if (diff.count() >= 1) {
        time = curr;
        value = frames / diff.count();
        frames = 0;
    }
}
