#ifndef APPFW_COLOR_H
#define APPFW_COLOR_H
#include <stddef.h>
#include <stdint.h>

namespace appfw {

class Color {
public:
    inline Color() {
        r() = 0;
        g() = 0;
        b() = 0;
        a() = 0;
    }

    inline Color(int _r, int _g, int _b, int _a = 255) {
        r() = (uint8_t)_r;
        g() = (uint8_t)_g;
        b() = (uint8_t)_b;
        a() = (uint8_t)_a;
    }

    // Ref getters
    inline uint8_t &r() { return m_Color[0]; }

    inline uint8_t &g() { return m_Color[1]; }

    inline uint8_t &b() { return m_Color[2]; }

    inline uint8_t &a() { return m_Color[3]; }

    inline uint8_t &operator[](size_t idx) { return m_Color[idx]; }

    // Const ref getters
    inline uint8_t r() const { return m_Color[0]; }

    inline uint8_t g() const { return m_Color[1]; }

    inline uint8_t b() const { return m_Color[2]; }

    inline uint8_t a() const { return m_Color[3]; }

    inline uint8_t operator[](size_t idx) const { return m_Color[idx]; }

private:
    uint8_t m_Color[4];
};

} // namespace appfw

#endif
