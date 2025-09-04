#ifndef GL_CRAFT_TIMER_H
#define GL_CRAFT_TIMER_H
#include <chrono>

class ScopedTimer {
public:
    explicit ScopedTimer(std::string name = "Timer");
    ~ScopedTimer();

    void stop() const;

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimePoint;
    std::string m_name;
};

#endif //GL_CRAFT_TIMER_H
