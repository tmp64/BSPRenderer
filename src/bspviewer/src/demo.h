#ifndef DEMO_H
#define DEMO_H
#include <appfw/utils.h>
#include <appfw/span.h>
#include <cstdint>
#include <string>
#include <vector>
#include <glm/glm.hpp>

constexpr uint32_t DEMO_VERSION = (1 << 24) | ('M' << 16) | ('E' << 8) | ('D');

struct DemoHeader {
    /**
     * Must be DEMO_VERSION
     */
    uint32_t nVersion;

    /**
     * Time of one tick in microseconds
     */
    uint32_t iTickTime;

    /**
     * Number of ticks
     */
    uint32_t iTickCount;
};

struct DemoTick {
    glm::vec3 vViewOrigin;
    glm::vec3 vViewAngles;
    float flFov;
};

class DemoFormatException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DemoReader {
public:
    DemoReader();

    void loadFromFile(const std::string &path);
    void loadFromBytes(appfw::span<uint8_t> data);

    void start(long long iCurTime);
    void stop();
    const DemoTick &getTick();
    bool hasTimeCome(long long iCurTime);
    void advance();
    bool isEnd();

private:
    long long m_iTickTime = 0;
    uint32_t m_iTickCount = 0;

    long long m_iStartTime = 0;
    uint32_t m_iCurTick = 0;

    std::vector<DemoTick> m_Ticks;
};

class DemoWriter {
public:
    void setTickRate(float ticksPerSecond);

    void start(long long iCurTime);
    void finish(const std::string &filename);
    bool hasTimeCome(long long iCurTime);
    void addFrame(const DemoTick &tick);

private:
    long long m_iStartTime = 0;
    long long m_iTickTime = 0;

    std::vector<DemoTick> m_Ticks;
};

#endif
