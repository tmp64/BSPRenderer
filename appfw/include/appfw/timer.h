#ifndef APPFW_TIMER_H
#define APPFW_TIMER_H
#include <chrono>

namespace appfw {

/**
 * A timer. Count time from start() to stop()
 * Credits: https://gist.github.com/mcleary/b0bf4fa88830ff7c882d
 */
class Timer {
public:
    inline void start() {
        m_StartTime = std::chrono::steady_clock::now();
        m_bRunning = true;
    }

    inline void stop() {
        m_EndTime = std::chrono::steady_clock::now();
        m_bRunning = false;
    }

    inline long long elapsedMilliseconds() {
        std::chrono::time_point<std::chrono::steady_clock> endTime;

        if (m_bRunning) {
            endTime = std::chrono::steady_clock::now();
        } else {
            endTime = m_EndTime;
        }

        return std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
    }

    inline double elapsedSeconds() { return elapsedMilliseconds() / 1000.0; }

private:
    std::chrono::time_point<std::chrono::steady_clock> m_StartTime;
    std::chrono::time_point<std::chrono::steady_clock> m_EndTime;
    bool m_bRunning = false;
};

} // namespace appfw

#endif