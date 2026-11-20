#pragma once

#include "OnePole.h"
#include "Shaper.h"

#include <cmath>
#include <cstdint>

namespace fd1st
{

struct StyleState
{
    float hp = 0, lp = 0, lp2 = 0, peak = 0;
    float dcx = 0, dcy = 0;
    float wowPh = 0;
    float ap[4] = {};
    float drop[2048] = {};
    int dropW = 0;
    float hold = 0;
    int holdLeft = 0;
    uint32_t rng = 1u;
};

inline float tpdf (StyleState& s)
{
    s.rng = s.rng * 1664525u + 1013904223u;
    const float a = (float) (s.rng >> 8) * (1.f / 16777216.f);
    s.rng = s.rng * 1664525u + 1013904223u;
    const float b = (float) (s.rng >> 8) * (1.f / 16777216.f);
    return a - b;
}

inline float foldback (float x)
{
    while (x > 1.f || x < -1.f)
    {
        if (x > 1.f)
            x = 2.f - x;
        if (x < -1.f)
            x = -2.f - x;
    }
    return x;
}

inline float triFold (float x)
{
    x = foldback (x);
    return x * 0.9f;
}

inline float allpass (float x, float& z, float a)
{
    const float y = -x * a + z;
    z = y * a + x;
    return y;
}

inline float quantize (float x, int levels)
{
    const float n = (float) (levels > 2 ? levels : 2);
    const float q = std::round ((x * 0.5f + 0.5f) * (n - 1.f)) / (n - 1.f);
    const float y = q * 2.f - 1.f;
    const float t = y - x;
    return x + t * t * t; // cubic tween toward bin
}

inline float crushBits (float x, float bits)
{
    const float st = std::pow (2.f, bits - 1.f);
    return std::round (x * st) / st;
}

inline float diodeShape (float x)
{
    const float ax = std::abs (x);
    if (ax < 1.f)
        return x - 0.33f * x * x * x;
    return std::copysign (0.67f + 0.1f * std::tanh (3.f * (ax - 1.f)), x);
}

// Instantaneous + light state. Delay-heavy styles use StyleState.
inline float shapeStyle (int id, float x, float drive01, float sr, StyleState& st)
{
    const float d = drive01 < 0.f ? 0.f : (drive01 > 1.f ? 1.f : drive01);
    float y = x;
    switch (id)
    {
        case 0:  y = std::tanh (x); break;
        case 1:  y = std::tanh (1.4f * x) - 0.08f * x * x * x + std::abs (x) * x * 0.05f; break;
        case 2:  y = std::tanh (x + 0.25f * x * x); break;
        case 3:  y = std::tanh (2.2f * x) + 0.12f * std::sin (3.f * x);
                 y = hp1 (y, st.hp, coeffAt (15.f, sr)); break;
        case 4:  y = x / (1.f + std::abs (x)); break;
        case 5:  y = std::tanh (x) + 0.04f * x * x;
                 y = lp1 (y, st.lp, coeffAt (12000.f, sr)); break;
        case 6:  y = std::tanh (1.8f * x) + 0.04f * x * x;
                 y = lp1 (y, st.lp, coeffAt (8000.f, sr)); break;
        case 7: {
            st.wowPh += 0.4f * 6.2831853f / sr;
            const float w = 1.f + 0.15f * 0.0003f * sr * std::sin (st.wowPh);
            y = std::tanh (1.8f * x * w) + 0.04f * x * x;
            y = lp1 (y, st.lp, coeffAt (8000.f, sr));
            y += 0.12f * lp1 (y, st.peak, coeffAt (2000.f, sr));
            break;
        }
        case 8:  y = hp1 (x, st.hp, coeffAt (80.f, sr));
                 y = std::tanh (1.2f * y);
                 y += 0.2f * (y - lp1 (y, st.lp, coeffAt (3500.f, sr))); break;
        case 9:  y = hp1 (x, st.hp, coeffAt (100.f, sr));
                 y = std::tanh (y);
                 y = std::tanh (1.6f * y);
                 y -= 0.12f * lp1 (y, st.lp, coeffAt (400.f, sr)); break;
        case 10: y = hp1 (x, st.hp, coeffAt (100.f, sr));
                 y = std::tanh (y);
                 y = std::tanh (2.4f * y);
                 y += 0.25f * lp1 (y, st.peak, coeffAt (1200.f, sr)); break;
        case 11: y = hp1 (x, st.hp, coeffAt (80.f, sr));
                 y = std::tanh (1.2f * y);
                 y += 0.2f * (y - lp1 (y, st.lp, coeffAt (6000.f, sr))); break;
        case 12: y = std::atan (x) * (2.f / 3.14159265f) * 1.2f;
                 y = lp1 (y, st.lp, coeffAt (7000.f, sr)); break;
        case 13: y = x / (1.f + std::pow (std::abs (x), 1.5f)); break;
        case 14: y = std::tanh (2.8f * x);
                 y += 0.3f * lp1 (y, st.peak, coeffAt (800.f, sr)); break;
        case 15: y = std::tanh (std::sin (x * 1.2f) * 2.f); break;
        case 16: y = hp1 (std::tanh (x), st.hp, coeffAt (40.f, sr)); break;
        case 17: y = dcBlock (x, st.dcx, st.dcy);
                 y = y / (1.f + 0.3f * std::abs (y)); break;
        case 18: y = dcBlock (x, st.dcx, st.dcy);
                 y = std::tanh (0.9f * y) + 0.03f * y * y; break;
        case 19: y = dcBlock (x, st.dcx, st.dcy);
                 y = std::tanh (1.3f * y + 0.2f * y * y); break;
        case 20: y = std::tanh (0.7f * x); break;
        case 21: y = std::atan (x) * (2.f / 3.14159265f); break;
        case 22: y = std::tanh (2.5f * x); break;
        case 23: y = std::sin (1.5707963f * (x < -1.f ? -1.f : (x > 1.f ? 1.f : x))); break;
        case 24: y = diodeShape (x); break;
        case 25: y = x + 0.4f * x * x * x + 0.15f * x * x * x * x * x;
                 y = std::tanh (y); break;
        case 26: y = triFold (x); break;
        case 27: y = quantize (x, (int) (3.f + 12.f * (1.f - d))); break;
        case 28: y = std::tanh (x);
                 y += tpdf (st) * 0.08f * std::abs (y); break;
        case 29: {
            float s = x;
            const float fb = 0.4f + 0.4f * d;
            s = allpass (s, st.ap[0], 0.3f);
            s = allpass (s, st.ap[1], 0.45f);
            s = allpass (s, st.ap[2], 0.55f);
            s = allpass (s, st.ap[3], 0.65f);
            y = std::tanh (x + s * fb);
            break;
        }
        case 30: {
            st.drop[st.dropW & 2047] = x;
            const float rate = 1.f - 0.35f * d;
            const float pos = (float) st.dropW - rate;
            const int i0 = ((int) pos) & 2047;
            const int i1 = (i0 + 1) & 2047;
            const float f = pos - std::floor (pos);
            const float rd = st.drop[i0] + (st.drop[i1] - st.drop[i0]) * f;
            ++st.dropW;
            y = std::tanh (x * 0.5f + rd * 0.5f);
            break;
        }
        case 31: y = foldback (x); break;
        case 32: y = hp1 (std::abs (x), st.hp, coeffAt (30.f, sr));
                 y = std::tanh (1.5f * y); break;
        case 33: {
            const float bits = 4.f + 12.f * (1.f - d);
            const int holdN = 1 + (int) (d * 32.f);
            if (st.holdLeft <= 0)
            {
                st.hold = crushBits (x, bits);
                st.holdLeft = holdN;
            }
            --st.holdLeft;
            y = st.hold;
            break;
        }
        default: y = std::tanh (x); break;
    }
    if (! std::isfinite (y))
        y = 0.f;
    return y;
}

inline float peakNorm (int id, float driveDb, float sr)
{
    StyleState st;
    const float g = dbToLin (driveDb);
    const float d01 = driveDb / 36.f;
    const int n = 480;
    const float w = 2.f * 3.14159265f * 200.f / sr;
    float pk = 0.0001f;
    for (int i = 0; i < n; ++i)
    {
        const float u = std::sin (w * (float) i) * g;
        const float y = std::abs (shapeStyle (id, u, d01, sr, st));
        if (y > pk)
            pk = y;
    }
    return 1.f / (pk > 1.f ? pk : 1.f);
}

inline float rmsNorm (int id, float driveDb, float sr)
{
    StyleState st0, st;
    const float g0 = 1.f;
    const float g = dbToLin (driveDb);
    const float d01 = driveDb / 36.f;
    const int n = 480;
    const float w = 2.f * 3.14159265f * 200.f / sr;
    double a0 = 0, a = 0;
    for (int i = 0; i < n; ++i)
    {
        const float s = std::sin (w * (float) i);
        const float y0 = shapeStyle (id, s * g0, 0.f, sr, st0);
        const float y = shapeStyle (id, s * g, d01, sr, st);
        a0 += (double) y0 * y0;
        a += (double) y * y;
    }
    const float r0 = (float) std::sqrt (a0 / n);
    const float r = (float) std::sqrt (a / n);
    return r0 / (r > 0.2f ? r : 0.2f);
}

inline float makeupGain (int id, float driveDb, float sr)
{
    static float tab[34][37];
    static int ready = 0;
    if (! ready)
    {
        for (int s = 0; s < 34; ++s)
            for (int i = 0; i < 37; ++i)
                tab[s][i] = (s <= 22) ? rmsNorm (s, (float) i, sr)
                                      : peakNorm (s, (float) i, sr);
        ready = 1;
    }
    const float t = driveDb < 0.f ? 0.f : (driveDb > 36.f ? 36.f : driveDb);
    const int i = (int) t;
    const int j = i < 36 ? i + 1 : 36;
    const float f = t - (float) i;
    return tab[id][i] + (tab[id][j] - tab[id][i]) * f;
}

} // namespace fd1st
