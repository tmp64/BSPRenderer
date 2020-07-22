#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H
#include <appfw/color.h>
#include <glm/glm.hpp>

inline glm::vec4 colorToVec(appfw::Color c) {
    return {
        c.r() / 255.f,
        c.g() / 255.f,
        c.b() / 255.f,
        c.a() / 255.f
    };
}

#endif
