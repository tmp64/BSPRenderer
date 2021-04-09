#include <renderer/time_smoother.h>

TimeSmoother::TimeSmoother() { memset(m_Buf, 0, sizeof(m_Buf)); }

void TimeSmoother::add(unsigned time) {
    m_uTotal -= m_Buf[m_uIndex];
    m_Buf[m_uIndex] = time;
    m_uTotal += time;
    m_uIndex++;

    if (m_uIndex == BUF_SIZE) {
        m_uIndex = 0;
    }
}
