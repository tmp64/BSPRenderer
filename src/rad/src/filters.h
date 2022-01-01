#ifndef FILTERS_H
#define FILTERS_H

namespace rad::filters {

template <typename T>
inline float cubic(float x) {
    constexpr float B = T::B;
    constexpr float C = T::C;

    float absx = std::abs(x);
    float val = 0;

    if (absx < 1.0f) {
        val = (12 - 9 * B - 6 * C) * absx * absx * absx;
        val += (-18 + 12 * B + 6 * C) * absx * absx;
        val += 6 - 2 * B;
    } else if (std::abs(x) < 2.0f) {
        val = (-B - 6 * C) * absx * absx * absx;
        val += (6 * B + 30 * C) * absx * absx;
        val += (-12 * B - 48 * C) * absx;
        val += 8 * B + 24 * C;
    } else {
        return 0;
    }

    return val / 6;
}

struct BSplineArgs {
    static constexpr float B = 1;
    static constexpr float C = 0;
};

//! B Spline cubic filter
//! https://entropymine.com/imageworsener/bicubic/
inline float bspline(float x) {
    return cubic<BSplineArgs>(x);
}

struct MitchellArgs {
    static constexpr float B = 1 / 3.0f;
    static constexpr float C = 1 / 3.0f;
};

// Mitchell cubic filter
// https://entropymine.com/imageworsener/bicubic/
inline float mitchell(float x) {
    return cubic<MitchellArgs>(x);
}

struct [[maybe_unused]] CatRomArgs {
    static constexpr float B = 0;
    static constexpr float C = 0.5f;
};

// Catmull-Rom cubic filter
// https://entropymine.com/imageworsener/bicubic/
inline float catrom(float x) {
    return cubic<CatRomArgs>(x);
}

// Linear filter, range [-1; 1]
inline float linear1(float x) {
    if (x < -1) {
        return 0;
    }

    if (x < 0) {
        return 1.0f + x;
    }

    if (x < 1) {
        return 1.0f - x;
    }

    return 0;
}

// Linear filter, range [-2; 2]
inline float linear2(float x) {
    if (x < -2) {
        return 0;
    }

    if (x < 0) {
        return 1.0f + 0.5f * x;
    }

    if (x < 2) {
        return 1.0f - 0.5f * x;
    }

    return 0;
}

} // namespace rad::filters

#endif
