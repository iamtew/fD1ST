#pragma once

#include "../parameters/ParameterIDs.h"
#include "Shaper.h"

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstring>

namespace fd1st
{

struct MotionMods
{
    float glob_mix = 0, glob_heat = 0, glob_spread = 0, glob_drive = 0, glob_blend = 0;
    float glob_fb_amt = 0, glob_comp = 0, glob_tone = 0, glob_out = 0, glob_motion = 0;
    float xover[5] {};
    float cDrive[6] {}, cMix[6] {}, cBias[6] {}, cPan[6] {}, cDyn[6] {}, cLevel[6] {};
    float cFbAmt[6] {}, cFiltHz[6] {}, cFiltQ[6] {};
};

struct Motion
{
    juce::ValueTree tree { "MOTION" };
    MotionMods mods;
    float orbPhase[4] {}, orbSnh[4] {}, nzSnh[2] {}, nzBrown[2] {}, nzPh[2] {};
    float stk[4] {}, fol[4] {}, folZ[4] {}, lagZ[50] {};
    int stkStage[4] {};
    float xy[6][2] {};
    float midiCC[128] {}, midiVel = 0, midiNote = 0, midiBend = 0, midiPress = 0;
    bool midiTrig = false;
    uint32_t rng = 1;

    void ensureChild (const juce::String& n, std::initializer_list<std::pair<const char*, juce::var>> props)
    {
        if (tree.getChildWithName (n).isValid())
            return;
        juce::ValueTree c (n);
        for (auto& p : props)
            c.setProperty (p.first, p.second, nullptr);
        tree.appendChild (c, nullptr);
    }

    void prepare()
    {
        for (int i = 0; i < 4; ++i)
        {
            ensureChild ("orb" + juce::String (i),
                         { { "wave", 0 }, { "morph", 0.0 }, { "rate_hz", 1.0 }, { "sync", false },
                           { "div", 2 }, { "phase", 0.0 }, { "freq_off", 0.0 }, { "balance", 0.0 },
                           { "trig", 0 } });
            auto o = tree.getChildWithName ("orb" + juce::String (i));
            if (! o.getChildWithName ("steps").isValid())
            {
                juce::ValueTree st ("steps");
                for (int k = 0; k < 16; ++k)
                    st.setProperty ("s" + juce::String (k), 0.0, nullptr);
                st.setProperty ("glide", 0.0, nullptr);
                o.appendChild (st, nullptr);
            }
            ensureChild ("stk" + juce::String (i),
                         { { "a", 0.01 }, { "d", 0.1 }, { "s", 0.7 }, { "r", 0.2 },
                           { "ca", 0.0 }, { "cd", 0.0 }, { "cr", 0.0 },
                           { "trig", 0 }, { "thresh", -24.0 }, { "range", 0 }, { "retrig", true } });
            ensureChild ("fol" + juce::String (i),
                         { { "mode", 0 }, { "atk", 10.0 }, { "rel", 100.0 }, { "thresh", -40.0 },
                           { "gain", 1.0 }, { "det_hz", 1000.0 }, { "det_width", 1.0 }, { "src", 0 } });
        }
        for (int i = 0; i < 2; ++i)
        {
            ensureChild ("mid" + juce::String (i), { { "kind", 0 }, { "cc", 1 } });
            ensureChild ("nz" + juce::String (i),
                         { { "kind", 0 }, { "rate_hz", 2.0 }, { "sync", false }, { "div", 2 }, { "smooth", 0.2 } });
        }
        for (int i = 0; i < 6; ++i)
            ensureChild ("xy" + juce::String (i), { { "x", 0.0 }, { "y", 0.0 }, { "unipolar", false }, { "slider", false } });
        for (int i = 0; i < 50; ++i)
            ensureChild ("wire" + juce::String (i),
                         { { "src", i == 0 ? "orb:0" : "" }, { "dst", juce::String() },
                           { "depth", 0.0 }, { "on", false }, { "lag", 0.0 } });
    }

    void pullFromTree() {}

    static float wave (int kind, float p, float snh, const juce::ValueTree& steps)
    {
        if (kind == 1)
            return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
        if (kind == 2)
            return 2.f * p - 1.f;
        if (kind == 3)
            return 1.f - 2.f * p;
        if (kind == 4)
            return p < 0.5f ? 1.f : -1.f;
        if (kind == 5)
            return snh;
        if (kind == 6)
        {
            const int i = juce::jlimit (0, 15, (int) (p * 16.f));
            const float a = (float) steps.getProperty ("s" + juce::String (i));
            const int j = (i + 1) & 15;
            const float b = (float) steps.getProperty ("s" + juce::String (j));
            const float g = (float) steps.getProperty ("glide");
            const float f = p * 16.f - (float) i;
            return a + (b - a) * (g * f);
        }
        return std::sin (p * 6.2831853f);
    }

    float noiseTick (int i, float dt, const juce::ValueTree& n)
    {
        const int kind = (int) n.getProperty ("kind");
        const float rate = juce::jlimit (0.01f, 40.f, (float) n.getProperty ("rate_hz"));
        const float sm = juce::jlimit (0.f, 1.f, (float) n.getProperty ("smooth"));
        rng = rng * 1664525u + 1013904223u;
        const float u = (float) (rng >> 8) * (1.f / 16777216.f) * 2.f - 1.f;
        float y = u;
        if (kind == 1)
        {
            nzPh[i] += rate * dt;
            y = std::sin (nzPh[i] * 6.2831853f + u * 0.3f);
        }
        else if (kind == 2)
        {
            nzPh[i] += rate * dt;
            if (nzPh[i] >= 1.f)
            {
                nzPh[i] -= 1.f;
                nzSnh[i] = u;
            }
            y = nzSnh[i];
        }
        else if (kind == 3)
        {
            nzBrown[i] = juce::jlimit (-1.f, 1.f, nzBrown[i] + u * 0.02f);
            y = nzBrown[i];
        }
        const float a = sm * sm * 0.95f + 0.05f;
        nzSnh[i] += (y - nzSnh[i]) * (1.f - a);
        return nzSnh[i];
    }

    float sourceValue (const juce::String& src, float dt, float bpm, float mainRms, float scRms, const float lensRms[6])
    {
        if (src.startsWith ("orb:"))
        {
            const int i = juce::jlimit (0, 3, src.getTrailingIntValue());
            auto o = tree.getChildWithName ("orb" + juce::String (i));
            float rate = juce::jlimit (0.01f, 40.f, (float) o.getProperty ("rate_hz"));
            if ((bool) o.getProperty ("sync"))
            {
                static const float beats[] = { 0.125f, 0.25f, 0.5f, 1.f, 2.f, 4.f };
                const int div = juce::jlimit (0, 5, (int) o.getProperty ("div"));
                const float t = beats[div] * 60.f / (float) bpm;
                rate = t > 1.0e-6f ? 1.f / t : rate;
            }
            orbPhase[i] += rate * dt;
            if (orbPhase[i] >= 1.f)
            {
                rng = rng * 1664525u + 1013904223u;
                orbSnh[i] = (float) (rng >> 8) * (1.f / 16777216.f) * 2.f - 1.f;
                orbPhase[i] -= std::floor (orbPhase[i]);
            }
            auto steps = o.getChildWithName ("steps");
            return wave ((int) o.getProperty ("wave"), orbPhase[i], orbSnh[i], steps);
        }
        if (src.startsWith ("stk:"))
        {
            const int i = juce::jlimit (0, 3, src.getTrailingIntValue());
            auto s = tree.getChildWithName ("stk" + juce::String (i));
            const int trig = (int) s.getProperty ("trig");
            const float th = fd1st::dbToLin ((float) s.getProperty ("thresh"));
            bool hit = false;
            if (trig == 0)
                hit = midiTrig;
            else if (trig == 1)
                hit = mainRms > th;
            else if (trig == 2)
                hit = scRms > th;
            else
                hit = lensRms[juce::jlimit (0, 5, trig - 3)] > th;
            const float A = juce::jmax (0.0001f, (float) s.getProperty ("a"));
            const float D = juce::jmax (0.001f, (float) s.getProperty ("d"));
            const float S = juce::jlimit (0.f, 1.f, (float) s.getProperty ("s"));
            const float R = juce::jmax (0.001f, (float) s.getProperty ("r"));
            if (hit && (stkStage[i] == 0 || (bool) s.getProperty ("retrig")))
                stkStage[i] = 1;
            if (stkStage[i] == 1)
            {
                stk[i] += dt / A;
                if (stk[i] >= 1.f)
                {
                    stk[i] = 1.f;
                    stkStage[i] = 2;
                }
            }
            else if (stkStage[i] == 2)
            {
                stk[i] -= dt / D * (1.f - S);
                if (stk[i] <= S)
                {
                    stk[i] = S;
                    stkStage[i] = hit ? 2 : 3;
                }
                if (! hit)
                    stkStage[i] = 3;
            }
            else if (stkStage[i] == 3)
            {
                stk[i] -= dt / R;
                if (stk[i] <= 0.f)
                {
                    stk[i] = 0.f;
                    stkStage[i] = 0;
                }
            }
            return stk[i] * 2.f - 1.f;
        }
        if (src.startsWith ("fol:"))
        {
            const int i = juce::jlimit (0, 3, src.getTrailingIntValue());
            auto f = tree.getChildWithName ("fol" + juce::String (i));
            const int srcI = (int) f.getProperty ("src");
            float x = mainRms;
            if (srcI == 1)
                x = scRms;
            else if (srcI >= 2)
                x = lensRms[juce::jlimit (0, 5, srcI - 2)];
            const float atk = juce::jmax (0.0005f, (float) f.getProperty ("atk") * 0.001f);
            const float rel = juce::jmax (0.005f, (float) f.getProperty ("rel") * 0.001f);
            const float a = x > fol[i] ? (1.f - std::exp (-dt / atk)) : (1.f - std::exp (-dt / rel));
            fol[i] += (x - fol[i]) * a;
            const float th = fd1st::dbToLin ((float) f.getProperty ("thresh"));
            float y = juce::jlimit (0.f, 1.f, (fol[i] - th) / (1.f - th + 1.0e-8f));
            y *= (float) f.getProperty ("gain");
            return juce::jlimit (-1.f, 1.f, y * 2.f - 1.f);
        }
        if (src.startsWith ("mid:"))
        {
            const int i = juce::jlimit (0, 1, src.getTrailingIntValue());
            auto m = tree.getChildWithName ("mid" + juce::String (i));
            const int kind = (int) m.getProperty ("kind");
            if (kind == 1)
                return midiVel * 2.f - 1.f;
            if (kind == 2)
                return midiNote * 2.f - 1.f;
            if (kind == 3)
                return midiBend;
            if (kind == 4)
                return midiPress * 2.f - 1.f;
            const int cc = juce::jlimit (0, 127, (int) m.getProperty ("cc"));
            return midiCC[cc] * 2.f - 1.f;
        }
        if (src.startsWith ("xy:"))
        {
            const int i = juce::jlimit (0, 5, src.getTrailingIntValue());
            auto x = tree.getChildWithName ("xy" + juce::String (i));
            xy[i][0] = (float) x.getProperty ("x");
            xy[i][1] = (float) x.getProperty ("y");
            return (bool) x.getProperty ("slider") ? xy[i][0] : xy[i][0];
        }
        if (src.startsWith ("nz:"))
        {
            const int i = juce::jlimit (0, 1, src.getTrailingIntValue());
            return noiseTick (i, dt, tree.getChildWithName ("nz" + juce::String (i)));
        }
        return 0.f;
    }

    void addDest (const juce::String& dst, float v)
    {
        if (dst == ParamID::glob_mix)
            mods.glob_mix += v;
        else if (dst == ParamID::glob_heat)
            mods.glob_heat += v;
        else if (dst == ParamID::glob_spread)
            mods.glob_spread += v;
        else if (dst == ParamID::glob_drive_db)
            mods.glob_drive += v;
        else if (dst == ParamID::glob_blend)
            mods.glob_blend += v;
        else if (dst == ParamID::glob_fb_amt)
            mods.glob_fb_amt += v;
        else if (dst == ParamID::glob_comp)
            mods.glob_comp += v;
        else if (dst == ParamID::glob_tone)
            mods.glob_tone += v;
        else if (dst == ParamID::glob_out_db)
            mods.glob_out += v;
        else
        {
            for (int i = 0; i < 6; ++i)
            {
                if (dst == ParamID::cDriveDb[i])
                    mods.cDrive[i] += v;
                else if (dst == ParamID::cMix[i])
                    mods.cMix[i] += v;
                else if (dst == ParamID::cBias[i])
                    mods.cBias[i] += v;
                else if (dst == ParamID::cPan[i])
                    mods.cPan[i] += v;
                else if (dst == ParamID::cDyn[i])
                    mods.cDyn[i] += v;
                else if (dst == ParamID::cLevelDb[i])
                    mods.cLevel[i] += v;
                else if (dst == ParamID::cFbAmt[i])
                    mods.cFbAmt[i] += v;
                else if (dst == ParamID::cFiltHz[i])
                    mods.cFiltHz[i] += v;
                else if (dst == ParamID::cFiltQ[i])
                    mods.cFiltQ[i] += v;
            }
            for (int i = 0; i < 5; ++i)
                if (dst == ParamID::xoverHz[i])
                    mods.xover[i] += v;
        }
    }

    void tick (float dt, float bpm, float globMotion, float mainRms, float scRms, const float lensRms[6])
    {
        mods = {};
        for (int i = 0; i < 50; ++i)
        {
            auto w = tree.getChildWithName ("wire" + juce::String (i));
            if (! (bool) w.getProperty ("on"))
                continue;
            const auto src = w.getProperty ("src").toString();
            const auto dst = w.getProperty ("dst").toString();
            if (src.isEmpty() || dst.isEmpty())
                continue;
            if (dst == ParamID::glob_os || dst == ParamID::glob_linphase || dst == ParamID::glob_route
                || dst == ParamID::glob_bands || dst == ParamID::glob_listen)
                continue;
            float v = sourceValue (src, dt, bpm, mainRms, scRms, lensRms);
            v *= (float) w.getProperty ("depth") * globMotion;
            const float lagMs = (float) w.getProperty ("lag");
            if (lagMs > 0.5f)
            {
                const float a = 1.f - std::exp (-dt / (lagMs * 0.001f));
                lagZ[i] += (v - lagZ[i]) * a;
                v = lagZ[i];
            }
            addDest (dst, v);
        }
    }

    // Back-compat for the old one-wire Mix path.
    bool wireOn = false;
    float wireDepth = 0;
    juce::String wireDst;
    float orbitSample (float dt)
    {
        float z[6] {};
        tick (dt, 120.f, 1.f, 0, 0, z);
        auto w = tree.getChildWithName ("wire0");
        wireOn = (bool) w.getProperty ("on");
        wireDepth = (float) w.getProperty ("depth");
        wireDst = w.getProperty ("dst").toString();
        return sourceValue ("orb:0", dt, 120.f, 0, 0, z);
    }
};

} // namespace fd1st
