#pragma once

#include "Biquad.h"
#include "Style.h"

#include <JuceHeader.h>
#include <cmath>
#include <vector>

namespace fd1st
{

class LensProcessor
{
public:
    void prepare (double sr, int)
    {
        sampleRate = sr > 1.0 ? sr : 48000.0;
        makeupGain (0, 0.f, (float) sampleRate); // fill shared table once
        ringCap = juce::jmax (64, (int) (sampleRate / 20.0) + 8);
        ring.assign ((size_t) ringCap, 0.f);
        filt.prepare ({ sampleRate, (juce::uint32) 64, 1 });
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
        levelSmooth.reset (sampleRate, 0.015);
        driveSmooth.setCurrentAndTargetValue (0.f);
        biasSmooth.setCurrentAndTargetValue (0.f);
        mixSmooth.setCurrentAndTargetValue (1.f);
        makeupSmooth.setCurrentAndTargetValue (1.f);
        levelSmooth.setCurrentAndTargetValue (1.f);
        env = 0;
        st = {};
        std::fill (ring.begin(), ring.end(), 0.f);
        ringW = 0;
        ringLp = 0;
        filt.reset();
        bass.reset();
        mid.reset();
        treble.reset();
        pres.reset();
        g = 1.f;
        b = 0.f;
        m = 1.f;
        mx = 1.f;
        lvl = 1.f;
        panL = 1.f;
        panR = 1.f;
        style = 0;
        on = true;
        mute = false;
        skipped = false;
    }

    bool wasSkipped() const { return skipped; }

    void configure (bool lensOn, bool lensMute, int styleId, float, float heat,
                    float filtHz, float filtQ, int fType, int fPos,
                    float bassDb, float midDb, float trebleDb, float presDb, bool safe)
    {
        on = lensOn;
        mute = lensMute;
        style = juce::jlimit (0, 33, styleId);
        heatDb = heat * 12.f;
        filtType = fType;
        filtPos = fPos;
        fbSafe = safe;
        if (filtType >= 1 && filtType <= 3)
        {
            using T = juce::dsp::StateVariableTPTFilterType;
            filt.setCutoffFrequency (juce::jlimit (20.f, 20000.f, filtHz));
            filt.setResonance (juce::jlimit (0.3f, 8.f, filtQ));
            filt.setType (filtType == 2 ? T::bandpass : (filtType == 3 ? T::highpass : T::lowpass));
        }
        const float sr = (float) sampleRate;
        bass.lowShelf (sr, 120.f, dbToLin (bassDb));
        mid.peak (sr, 800.f, 0.8f, dbToLin (midDb));
        treble.highShelf (sr, 4000.f, dbToLin (trebleDb));
        pres.peak (sr, 8000.f, 1.2f, dbToLin (presDb));
    }

    void tick (float driveDb, float bias, float mix, float levelDb, float pan,
               float ringAmt, float ringHz, float dynAmt)
    {
        skipped = ! on && ! mute;
        driveSmooth.setTargetValue (juce::jlimit (0.f, 36.f, driveDb + heatDb));
        biasSmooth.setTargetValue (juce::jlimit (-1.f, 1.f, bias));
        mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, mix));
        const float d = driveSmooth.getNextValue();
        g = dbToLin (d);
        b = biasSmooth.getNextValue();
        mx = mixSmooth.getNextValue();
        makeupSmooth.setTargetValue (makeupGain (style, d, (float) sampleRate));
        m = makeupSmooth.getNextValue();
        levelSmooth.setTargetValue (juce::Decibels::decibelsToGain (juce::jlimit (-60.f, 36.f, levelDb)));
        lvl = levelSmooth.getNextValue();
        const float p = juce::jlimit (-1.f, 1.f, pan);
        panL = p <= 0.f ? 1.f : 1.f - p;
        panR = p >= 0.f ? 1.f : 1.f + p;
        fbAmt = fbSafe ? juce::jmin (0.85f, ringAmt) : juce::jlimit (0.f, 0.95f, ringAmt);
        fbHz = juce::jlimit (20.f, 4000.f, ringHz);
        dyn = dynAmt;
        drive01 = juce::jlimit (0.f, 1.f, d / 36.f);
    }

    float apply (float x, int channel)
    {
        if (mute)
            return 0.f;
        if (! on)
            return x;

        const float dry = x;
        x = x + readRing() * fbAmt;
        if (dyn != 0.f)
            x *= dynGain (x);
        if (filtType >= 1 && filtType <= 3 && filtPos == 0)
            x = filt.processSample (0, x);

        float y = shapeStyle (style, x * g + b * 0.25f, drive01, (float) sampleRate, st);
        y = juce::jlimit (-8.f, 8.f, y) * m;
        if (! std::isfinite (y))
            y = 0.f;

        if (filtType >= 1 && filtType <= 3 && filtPos == 1)
            y = filt.processSample (0, y);

        y = pres.process (treble.process (mid.process (bass.process (y))));
        y = dry + (y - dry) * mx;
        y *= lvl * (channel == 0 ? panL : panR);
        writeRing (y);
        return y;
    }

    static float transfer (int styleId, float x, float driveDb, float heat, float bias, float sr)
    {
        StyleState local;
        const float d = juce::jlimit (0.f, 36.f, driveDb + heat * 12.f);
        float y = shapeStyle (styleId, x * dbToLin (d) + bias * 0.25f, d / 36.f, sr, local);
        y = juce::jlimit (-8.f, 8.f, y);
        y *= makeupGain (styleId, d, sr);
        return y;
    }

private:
    double sampleRate = 48000.0;
    juce::SmoothedValue<float> driveSmooth, biasSmooth, mixSmooth, makeupSmooth, levelSmooth;
    StyleState st;
    std::vector<float> ring;
    int ringCap = 64, ringW = 0;
    float ringLp = 0, fbAmt = 0, fbHz = 220.f, heatDb = 0, dyn = 0, env = 0, drive01 = 0;
    float g = 1.f, b = 0.f, m = 1.f, mx = 1.f, lvl = 1.f, panL = 1.f, panR = 1.f;
    int style = 0, filtType = 0, filtPos = 1;
    bool on = true, mute = false, fbSafe = true, skipped = false;
    juce::dsp::StateVariableTPTFilter<float> filt;
    Biquad bass, mid, treble, pres;

    float readRing() const
    {
        if (ringCap < 2 || fbAmt <= 0.0001f)
            return 0.f;
        const int del = juce::jlimit (1, ringCap - 1, (int) (sampleRate / (double) fbHz));
        return ring[(size_t) ((ringW - del + ringCap) % ringCap)];
    }

    void writeRing (float y)
    {
        if (ringCap < 2)
            return;
        float v = y;
        if (v > 0.98f)
            v = 0.98f;
        if (v < -0.98f)
            v = -0.98f;
        v = lp1 (v, ringLp, coeffAt (6000.f, (float) sampleRate));
        ring[(size_t) ringW] = v;
        ringW = (ringW + 1) % ringCap;
    }

    float dynGain (float x)
    {
        const float ax = std::abs (x);
        const float coef = ax > env ? 0.08f : 0.008f;
        env += (ax - env) * coef;
        if (dyn > 0.f)
        {
            const float th = dbToLin (-24.f);
            const float ratio = 1.f + 7.f * dyn;
            float gr = 1.f;
            if (env > th)
                gr = std::pow (th / (env + 1.0e-8f), 1.f - 1.f / ratio);
            return gr * (1.f + 0.4f * dyn);
        }
        const float th = dbToLin (-36.f);
        if (env >= th)
            return 1.f;
        const float floor = dbToLin (-24.f * (-dyn));
        return juce::jmax (floor, env / (th + 1.0e-8f));
    }
};

} // namespace fd1st
