#ifndef SLIM_ENGINE_TIME_H
#define SLIM_ENGINE_TIME_H

#include <cmath>
#include <string>
#include <chrono>
#include <optional>

class Time {
    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    std::chrono::time_point<std::chrono::high_resolution_clock> clock;
public:
    Time();
    virtual ~Time();

    void Update();
    double Delta() const;
    double Elapsed() const;
};

class FPS {
    std::chrono::time_point<std::chrono::system_clock> time;
    int frames = 0;
    double value = 0.0;
public:
    FPS();
    virtual ~FPS();
    double GetValue() const { return value; }
    void Update();
};

#endif // SLIM_ENGINE_TIME_H
