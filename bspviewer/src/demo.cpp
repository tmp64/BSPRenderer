#include <array>
#include <appfw/dbg.h>
#include <fmt/format.h>
#include <fstream>
#include "demo.h"

DemoReader::DemoReader() {}

void DemoReader::loadFromFile(const std::string &filename) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file.fail()) {
        throw std::runtime_error(std::string("Failed to open file: ") + strerror(errno));
    }

    std::vector<uint8_t> data;
    data.reserve(4 * 1024 * 1024); // Set capacity to 4 MB

    for (;;) {
        uint8_t c;
        file.read((char *)&c, 1);
        if (file.eof()) {
            break;
        }
        data.push_back(c);
    }

    file.close();
    data.shrink_to_fit();

    loadFromBytes(data);
}

void DemoReader::loadFromBytes(appfw::span<uint8_t> data) {
    if (data.empty()) {
        throw DemoFormatException("File is empty");
    }

    // Read header
    DemoHeader header;

    if (data.size() < sizeof(DemoHeader)) {
        throw DemoFormatException("File is too small for header");
    }

    memcpy(&header, data.data(), sizeof(DemoHeader));

    if (header.nVersion != DEMO_VERSION) {
        throw DemoFormatException(
            fmt::format("Version is invalid. Expected {}, got {}", DEMO_VERSION, header.nVersion));
    }

    m_iTickCount = header.iTickCount;
    m_iTickTime = header.iTickTime;

    // Read array
    size_t size = data.size() - sizeof(DemoHeader);
    if (size != sizeof(DemoTick) * header.iTickCount) {
        throw DemoFormatException("Invalid file size");
    }

    m_Ticks.resize(header.iTickCount);
    memcpy(m_Ticks.data(), data.data() + sizeof(DemoHeader), size);
}

void DemoReader::start(long long iCurTime) {
    m_iCurTick = 0;
    m_iStartTime = iCurTime;
}

void DemoReader::stop() { m_iCurTick = m_iTickCount; }

const DemoTick &DemoReader::getTick() {
    AFW_ASSERT(!m_Ticks.empty());
    return m_Ticks[m_iCurTick];
}

bool DemoReader::hasTimeCome(long long iCurTime) { 
    return iCurTime >= (m_iStartTime + m_iCurTick * m_iTickTime);
}

void DemoReader::advance() { 
    if (!isEnd()) {
        m_iCurTick++;
    }
}

bool DemoReader::isEnd() { 
    AFW_ASSERT(m_iCurTick <= m_iTickCount);
    return m_iCurTick == m_iTickCount;
}

void DemoWriter::setTickRate(float ticksPerSecond) { m_iTickTime = (long long)(1000000 / ticksPerSecond); }

void DemoWriter::start(long long iCurTime) {
    m_iStartTime = iCurTime;
    m_Ticks.clear();
    m_Ticks.reserve(1024 * 1024); // 72 MB
}

void DemoWriter::finish(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::out);

    auto fnWriteStruct = [&](const auto &data) {
        using T = std::remove_reference<decltype(data)>::type;
        std::array<uint8_t, sizeof(T)> rawData;
        memcpy(rawData.data(), &data, sizeof(T));

        file.write(reinterpret_cast<const char *>(rawData.data()), rawData.size());
    };

    DemoHeader header;
    header.nVersion = DEMO_VERSION;
    header.iTickTime = (uint32_t)m_iTickTime;
    header.iTickCount = (uint32_t)m_Ticks.size();

    fnWriteStruct(header);

    for (const DemoTick &tick : m_Ticks) {
        fnWriteStruct(tick);
    }

    file.close();
}

bool DemoWriter::hasTimeCome(long long iCurTime) { return iCurTime >= (m_iStartTime + (long long)m_Ticks.size() * m_iTickTime); }

void DemoWriter::addFrame(const DemoTick &tick) { m_Ticks.push_back(tick); }
