#pragma once

#include <cmath>

namespace fd1st
{

inline float lp1 (float x, float& z, float a)
{
    z += a * (x - z);
    return z;
}

inline float hp1 (float x, float& z, float a)
{
    return x - lp1 (x, z, a);
}

inline float dcBlock (float x, float& xz, float& yz, float r = 0.995f)
{
    const float y = x - xz + r * yz;
    xz = x;
    yz = y;
    return y;
}

inline float coeffAt (float hz, float sr)
{
    const float ny = 0.45f * sr;
    const float f = hz < 1.f ? 1.f : (hz > ny ? ny : hz);
    const float a = 1.f - (float) std::exp (-2.0 * 3.14159265358979323846 * (double) f / (double) sr);
    return a < 0.0001f ? 0.0001f : (a > 1.f ? 1.f : a);
}

} // namespace fd1st
