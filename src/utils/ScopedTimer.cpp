#include "ScopedTimer.h"

#include <iostream>

ScopedTimer::ScopedTimer(std::string name) : m_name(std::move(name)) {
    m_startTimePoint = std::chrono::high_resolution_clock::now();
}

ScopedTimer::~ScopedTimer() {
    stop();
}

void ScopedTimer::stop() const {
    const auto endTimePoint = std::chrono::high_resolution_clock::now();

    const auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimePoint).time_since_epoch().
            count();
    const auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();

    const auto duration = end - start;
    const double ms = static_cast<double>(duration) * 0.001;

    std::cout << "[" << m_name << "] " << duration << "us (" << ms << "ms)\n";
}
