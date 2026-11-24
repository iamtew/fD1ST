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
    int applyCount = 0;

    void prepare (double sr, int)
    {
        sampleRate = sr > 1.0 ? sr : 48000.0;
        makeupGain (0, 0.f, (float) sampleRate);
        ringCap = juce::jmax (64, (int) (sampleRate / 20.0) + 8);
        ring.assign ((size_t) ringCap, 0.f);
        comb.assign ((size_t) ringCap, 0.f);
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) 64, 1 };
        filt.prepare (spec);
        filtBp.prepare (spec);
        filtHp.prepare (spec);
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
        std::fill (comb.begin(), comb.end(), 0.f);
        ringW = combW = 0;
        ringLp = 0;
        hold = 0;
        holdLeft = 0;
        filt.reset();
        filtBp.reset();
        filtHp.reset();
        peakBq.reset();
        bass.reset();
        mid.reset();
        treble.reset();
        pres.reset();
        g = 1.f;
        dp = 0.f;
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
        eqOn = false;
        lastFiltType = -1;
        applyCount = 0;
    }

    bool wasSkipped() const { return skipped; }

    void configure (bool lensOn, bool lensMute, int styleId,
                    float hzIn, float qIn, int fType, int fPos,
                    float bassDb, float midDb, float trebleDb, float presDb, bool safe)
    {
        on = lensOn;
        mute = lensMute;
        style = juce::jlimit (0, 33, styleId);
        filtType = fType;
        filtPos = fPos;
        fbSafe = safe;
        filtHz = juce::jlimit (20.f, 20000.f, hzIn);
        filtQ = juce::jlimit (0.3f, 8.f, qIn);
        using T = juce::dsp::StateVariableTPTFilterType;
        if (filtType >= 1 && filtType <= 3)
        {
            filt.setCutoffFrequency (filtHz);
            filt.setResonance (filtQ);
            if (filtType != lastFiltType)
            {
                if (filtType == 1)
                    filt.setType (T::lowpass);
                else if (filtType == 2)
                    filt.setType (T::bandpass);
                else
                    filt.setType (T::highpass);
            }
        }
        else if (filtType == 4)
            peakBq.notch ((float) sampleRate, filtHz, filtQ);
        else if (filtType == 5)
        {
            const float t = (filtQ - 0.3f) / 7.7f;
            peakBq.peak ((float) sampleRate, filtHz, 0.707f, dbToLin ((t * 2.f - 1.f) * 12.f));
        }
        else if (filtType == 6)
        {
            filt.setCutoffFrequency (this->filtHz);
            filtBp.setCutoffFrequency (this->filtHz);
            filtHp.setCutoffFrequency (this->filtHz);
            filt.setResonance (0.707f);
            filtBp.setResonance (0.707f);
            filtHp.setResonance (0.707f);
            if (filtType != lastFiltType)
            {
                filt.setType (T::lowpass);
                filtBp.setType (T::bandpass);
                filtHp.setType (T::highpass);
            }
        }
        lastFiltType = filtType;
        eqOn = std::abs (bassDb) > 0.01f || std::abs (midDb) > 0.01f
            || std::abs (trebleDb) > 0.01f || std::abs (presDb) > 0.01f;
        if (eqOn)
        {
            const float sr = (float) sampleRate;
            bass.lowShelf (sr, 120.f, dbToLin (bassDb));
            mid.peak (sr, 800.f, 0.8f, dbToLin (midDb));
            treble.highShelf (sr, 4000.f, dbToLin (trebleDb));
            pres.peak (sr, 8000.f, 1.2f, dbToLin (presDb));
        }
    }

    void tick (float driveDb, float heat, float drivePan, float bias, float mix, float levelDb, float pan,
               float ringAmt, float ringHz, float dynAmt)
    {
        skipped = ! on && ! mute;
        heatDb = heat * 12.f;
        driveSmooth.setTargetValue (juce::jlimit (0.f, 36.f, driveDb + heatDb));
        biasSmooth.setTargetValue (juce::jlimit (-1.f, 1.f, bias));
        mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, mix));
        const float d = driveSmooth.getNextValue();
        g = dbToLin (d);
        dp = juce::jlimit (-1.f, 1.f, drivePan);
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
        ++applyCount;
        if (mute)
            return 0.f;
        if (! on)
            return x;

        const float dry = x;
        x = x + readRing() * fbAmt;
        if (dyn != 0.f)
            x *= dynGain (x);
        if (filtPos == 0)
            x = filterSample (x);

        const float gp = g * (channel == 0 ? (1.f - dp) : (1.f + dp));
        float y = shapeStyle (style, x * gp + b * 0.25f, drive01, (float) sampleRate, st);
        y = juce::jlimit (-8.f, 8.f, y) * m;
        if (! std::isfinite (y))
            y = 0.f;

        if (filtPos != 0)
            y = filterSample (y);

        if (eqOn)
            y = pres.process (treble.process (mid.process (bass.process (y))));
        y = dry + (y - dry) * mx;
        y *= lvl * (channel == 0 ? panL : panR);
        if (fbAmt > 0.0001f)
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
    std::vector<float> ring, comb;
    int ringCap = 64, ringW = 0, combW = 0;
    float ringLp = 0, fbAmt = 0, fbHz = 220.f, heatDb = 0, dyn = 0, env = 0, drive01 = 0;
    float g = 1.f, dp = 0.f, b = 0.f, m = 1.f, mx = 1.f, lvl = 1.f, panL = 1.f, panR = 1.f;
    float filtHz = 1200.f, filtQ = 0.707f, hold = 0;
    int holdLeft = 0;
    int style = 0, filtType = 0, filtPos = 1, lastFiltType = -1;
    bool on = true, mute = false, fbSafe = true, skipped = false, eqOn = false;
    juce::dsp::StateVariableTPTFilter<float> filt, filtBp, filtHp;
    Biquad peakBq, bass, mid, treble, pres;

    float filterSample (float x)
    {
        if (filtType <= 0)
            return x;
        if (filtType >= 1 && filtType <= 3)
            return filt.processSample (0, x);
        if (filtType == 4 || filtType == 5)
            return peakBq.process (x);
        if (filtType == 6)
        {
            const float n = juce::jlimit (0.f, 1.f, (filtQ - 0.3f) / 7.7f);
            const float lp = filt.processSample (0, x);
            const float bp = filtBp.processSample (0, x);
            const float hp = filtHp.processSample (0, x);
            if (n < 0.5f)
            {
                const float t = n * 2.f;
                return lp + (bp - lp) * t;
            }
            const float t = (n - 0.5f) * 2.f;
            return bp + (hp - bp) * t;
        }
        if (filtType == 7)
        {
            const int del = juce::jlimit (1, ringCap - 1, (int) (sampleRate / (double) filtHz));
            const float delayed = comb[(size_t) ((combW - del + ringCap) % ringCap)];
            const float mix = juce::jlimit (0.f, 1.f, (filtQ - 0.3f) / 7.7f);
            comb[(size_t) combW] = x;
            combW = (combW + 1) % ringCap;
            return x + delayed * mix;
        }
        // Resample: S&H at filt_hz
        const int period = juce::jmax (1, (int) (sampleRate / (double) filtHz));
        if (holdLeft <= 0)
        {
            hold = x;
            holdLeft = period;
        }
        --holdLeft;
        return hold;
    }

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
