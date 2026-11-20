#pragma once

#include "OnePole.h"
#include "Shaper.h"

#include <cmath>

namespace fd1st
{

// Shared cavity-loop and output compressor. Amount 0 = bypass.
struct Comp
{
    float env = 0, detZ = 0;

    void reset() { env = 0; detZ = 0; }

    float gain (float detect, float amt, float sr)
    {
        if (amt <= 1.0e-5f)
            return 1.f;
        const float ax = std::abs (detect);
        const float atk = 1.f - (float) std::exp (-1.0 / (0.005 * (double) sr));
        const float rel = 1.f - (float) std::exp (-1.0 / (0.080 * (double) sr));
        env += (ax - env) * (ax > env ? atk : rel);
        const float th = dbToLin (-18.f);
        const float ratio = 1.f + 7.f * amt;
        if (env <= th)
            return 1.f;
        return std::pow (th / (env + 1.0e-8f), 1.f - 1.f / ratio);
    }

    float process (float x, float amt, float sr, bool hpf)
    {
        float d = x;
        if (hpf)
            d = hp1 (x, detZ, coeffAt (120.f, sr));
        return x * gain (d, amt, sr);
    }
};

inline float hermite4 (float y0, float y1, float y2, float y3, float t)
{
    const float c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * t + c2) * t + c1) * t + y1;
}

inline float readHermite (const float* buf, int cap, int w, float delaySamp)
{
    if (cap < 4)
        return 0.f;
    if (delaySamp < 1.f)
        delaySamp = 1.f;
    if (delaySamp > (float) (cap - 3))
        delaySamp = (float) (cap - 3);
    const int i1 = (int) delaySamp;
    const float t = delaySamp - (float) i1;
    auto at = [&] (int k) -> float
    {
        int p = w - k;
        p %= cap;
        if (p < 0)
            p += cap;
        return buf[p];
    };
    return hermite4 (at (i1 + 1), at (i1), at (i1 - 1), at (i1 - 2), t);
}

inline float tilt (float x, float& z, float a, float amount)
{
    const float lp = lp1 (x, z, a);
    const float gLo = dbToLin (-amount * 8.f);
    const float gHi = dbToLin (amount * 8.f);
    return lp * gLo + (x - lp) * gHi;
}

} // namespace fd1st
