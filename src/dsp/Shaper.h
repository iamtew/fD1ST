#pragma once

// JUCE-free shapers + Glow makeup. fd1st_check compiles this with cl.

#include <algorithm>
#include <cmath>

namespace fd1st
{

inline float dbToLin (float db)
{
    return std::pow (10.f, db * 0.05f);
}

inline float glowShape (float x)
{
    return std::tanh (x);
}

// RMS of Glow (probeAmp * 200 Hz sine * drive) over two periods at 48 kHz.
inline float glowProbeRms (float driveDb, float probeAmp, float sr = 48000.f)
{
    const float g = dbToLin (driveDb);
    const int n = 480; // two cycles of 200 Hz at 48 kHz
    const float w = 2.f * 3.14159265358979323846f * 200.f / sr;
    double acc = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const float y = glowShape (probeAmp * std::sin (w * (float) i) * g);
        acc += (double) y * y;
    }
    return (float) std::sqrt (acc / (double) n);
}

// Scale so a 0 dBFS 200 Hz sine keeps Glow RMS near the drive=0 reading.
inline float glowMakeup (float driveDb)
{
    const float rms0 = glowProbeRms (0.f, 1.f);
    const float rms = glowProbeRms (driveDb, 1.f);
    return rms0 / std::max (0.2f, rms);
}

} // namespace fd1st
