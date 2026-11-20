#pragma once

#include <cmath>

namespace fd1st
{

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0; }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

    void notch (float sr, float hz, float q)
    {
        const float w = 2.f * 3.14159265f * hz / sr;
        const float c = std::cos (w);
        const float s = std::sin (w);
        const float alpha = s / (2.f * q);
        const float a0 = 1.f + alpha;
        b0 = 1.f / a0;
        b1 = -2.f * c / a0;
        b2 = 1.f / a0;
        a1 = -2.f * c / a0;
        a2 = (1.f - alpha) / a0;
    }

    void peak (float sr, float hz, float q, float gainLin)
    {
        const float w = 2.f * 3.14159265f * hz / sr;
        const float c = std::cos (w);
        const float s = std::sin (w);
        const float A = std::sqrt (gainLin);
        const float alpha = s / (2.f * q);
        const float a0 = 1.f + alpha / A;
        b0 = (1.f + alpha * A) / a0;
        b1 = -2.f * c / a0;
        b2 = (1.f - alpha * A) / a0;
        a1 = -2.f * c / a0;
        a2 = (1.f - alpha / A) / a0;
    }

    void lowShelf (float sr, float hz, float gainLin)
    {
        const float w = 2.f * 3.14159265f * hz / sr;
        const float c = std::cos (w);
        const float A = std::sqrt (gainLin);
        const float s = std::sin (w);
        const float beta = std::sqrt (2.f * A);
        const float a0 = (A + 1.f) + (A - 1.f) * c + beta * s;
        b0 = A * ((A + 1.f) - (A - 1.f) * c + beta * s) / a0;
        b1 = 2.f * A * ((A - 1.f) - (A + 1.f) * c) / a0;
        b2 = A * ((A + 1.f) - (A - 1.f) * c - beta * s) / a0;
        a1 = -2.f * ((A - 1.f) + (A + 1.f) * c) / a0;
        a2 = ((A + 1.f) + (A - 1.f) * c - beta * s) / a0;
    }

    void highShelf (float sr, float hz, float gainLin)
    {
        const float w = 2.f * 3.14159265f * hz / sr;
        const float c = std::cos (w);
        const float A = std::sqrt (gainLin);
        const float s = std::sin (w);
        const float beta = std::sqrt (2.f * A);
        const float a0 = (A + 1.f) - (A - 1.f) * c + beta * s;
        b0 = A * ((A + 1.f) + (A - 1.f) * c + beta * s) / a0;
        b1 = -2.f * A * ((A - 1.f) + (A + 1.f) * c) / a0;
        b2 = A * ((A + 1.f) + (A - 1.f) * c - beta * s) / a0;
        a1 = 2.f * ((A - 1.f) - (A + 1.f) * c) / a0;
        a2 = ((A + 1.f) - (A - 1.f) * c - beta * s) / a0;
    }
};

} // namespace fd1st
