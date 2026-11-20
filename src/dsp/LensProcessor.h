#pragma once

#include "Shaper.h"

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace fd1st
{

// One lens. Step 2: Glow, drive, bias, makeup, mix. Other styles/routes later.
class LensProcessor
{
public:
    void prepare (double sr, int)
    {
        sampleRate = sr > 1.0 ? sr : 48000.0;
        fillMakeup();
        reset();
    }

    void setRate (double sr)
    {
        sampleRate = sr > 1.0 ? sr : 48000.0;
        reset();
    }

    void reset()
    {
        driveSmooth.reset (sampleRate, 0.015);
        biasSmooth.reset (sampleRate, 0.015);
        mixSmooth.reset (sampleRate, 0.015);
        makeupSmooth.reset (sampleRate, 0.015);
        driveSmooth.setCurrentAndTargetValue (0.f);
        biasSmooth.setCurrentAndTargetValue (0.f);
        mixSmooth.setCurrentAndTargetValue (1.f);
        makeupSmooth.setCurrentAndTargetValue (1.f);
        g = 1.f;
        b = 0.f;
        m = 1.f;
        mx = 1.f;
        on = true;
        mute = false;
    }

    void tick (bool lensOn, bool lensMute, float driveDb, float bias, float mix)
    {
        on = lensOn;
        mute = lensMute;
        driveSmooth.setTargetValue (juce::jlimit (0.f, 36.f, driveDb));
        biasSmooth.setTargetValue (juce::jlimit (-1.f, 1.f, bias));
        mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, mix));
        const float d = driveSmooth.getNextValue();
        g = dbToLin (d);
        b = biasSmooth.getNextValue();
        mx = mixSmooth.getNextValue();
        makeupSmooth.setTargetValue (makeupAt (d));
        m = makeupSmooth.getNextValue();
    }

    float apply (float x) const
    {
        if (mute)
            return 0.f;
        if (! on)
            return x;
        float y = glowShape (x * g + b * 0.25f);
        y = juce::jlimit (-8.f, 8.f, y) * m;
        if (! std::isfinite (y))
            y = 0.f;
        return x + (y - x) * mx;
    }

private:
    static constexpr int kMakeupN = 73; // 0..36 dB in 0.5 dB
    std::array<float, kMakeupN> makeup {};
    double sampleRate = 48000.0;
    juce::SmoothedValue<float> driveSmooth, biasSmooth, mixSmooth, makeupSmooth;
    float g = 1.f, b = 0.f, m = 1.f, mx = 1.f;
    bool on = true, mute = false;

    void fillMakeup()
    {
        for (int i = 0; i < kMakeupN; ++i)
            makeup[(size_t) i] = glowMakeup ((float) i * 0.5f);
    }

    float makeupAt (float driveDb) const
    {
        const float t = juce::jlimit (0.f, 36.f, driveDb) * 2.f;
        const int i = (int) t;
        const int j = std::min (i + 1, kMakeupN - 1);
        const float f = t - (float) i;
        return makeup[(size_t) i] + (makeup[(size_t) j] - makeup[(size_t) i]) * f;
    }
};

} // namespace fd1st
