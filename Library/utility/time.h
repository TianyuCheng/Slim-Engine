#ifndef SLIM_UTILITY_TIME_H
#define SLIM_UTILITY_TIME_H

#include <cmath>
#include <string>
#include <chrono>
#include "core/input.h"

namespace slim {

    class Time {
    public:
        explicit Time();
        virtual ~Time() = default;

        void Update();
        double Delta() const;
        double Elapsed() const;

    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> start;
        std::chrono::time_point<std::chrono::high_resolution_clock> clock;
    };

    class FPS {
    public:
        explicit FPS();
        virtual ~FPS() = default;
        double GetValue() const { return value; }
        void Update();

    private:
        std::chrono::time_point<std::chrono::system_clock> time;
        int frames = 0;
        double value = 0.0;
    };

}; // end of slim namespace

#endif // SLIM_UTILITY_TIME_H
