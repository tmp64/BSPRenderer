#ifndef RENDERER_TIME_SMOOTHER_H
#define RENDERER_TIME_SMOOTHER_H
#include <array>

class TimeSmoother {
public:
    static constexpr unsigned BUF_SIZE = 64;

    TimeSmoother();

    void add(unsigned time);

    inline unsigned getSmoothed() const { return (unsigned)(m_uTotal / BUF_SIZE); }

    inline TimeSmoother &operator+=(unsigned rhs) {
        add(rhs);
        return *this;
    }

private:
    unsigned long long m_uTotal = 0;
    unsigned m_Buf[BUF_SIZE];
    unsigned m_uIndex = 0;
};

#endif
