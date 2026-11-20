#pragma once

#include <cmath>

namespace fd1st
{

// Hard clip at ±1 with 0.1 dB cubic softening into the rail.
inline float railClip (float x)
{
    constexpr float t = 0.9885530946569389f; // 10^(-0.1/20)
    const float ax = std::abs (x);
    if (ax <= t)
        return x;
    if (ax >= 1.f)
        return std::copysign (1.f, x);
    const float n = (ax - t) / (1.f - t);
    const float y = t + (1.f - t) * (n - n * n * n * (1.f / 3.f));
    return std::copysign (y, x);
}

} // namespace fd1st
