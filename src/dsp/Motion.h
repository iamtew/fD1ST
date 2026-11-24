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

// Audio-thread snapshot. pullFromTree copies the ValueTree here once per host block.
enum { kSrcOrb, kSrcStk, kSrcFol, kSrcMid, kSrcXy, kSrcNz };
enum
{
    kDstMix, kDstHeat, kDstSpread, kDstDrive, kDstBlend,
    kDstFbAmt, kDstComp, kDstTone, kDstOut,
    kDstCDrive, kDstCMix, kDstCBias, kDstCPan, kDstCDyn, kDstCLevel,
    kDstCFbAmt, kDstCFiltHz, kDstCFiltQ, kDstXover
};

struct WireSnap
{
    int srcKind = 0, srcIdx = 0, dstKind = 0, dstIdx = 0, slot = 0;
    float depth = 0, lag = 0;
};

struct OrbSnap
{
    int wave = 0, div = 2;
    float rateHz = 1.f, steps[16] {}, glide = 0;
    bool sync = false;
};

struct StkSnap
{
    int trig = 0;
    float a = 0.01f, d = 0.1f, s = 0.7f, r = 0.2f, thresh = -24.f;
    bool retrig = true;
};

struct FolSnap
{
    int src = 0;
    float atk = 10.f, rel = 100.f, thresh = -40.f, gain = 1.f;
};

struct MidSnap { int kind = 0, cc = 1; };
struct NzSnap { int kind = 0; float rateHz = 2.f, smooth = 0.2f; };
struct XySnap { float x = 0, y = 0; };

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

    juce::Identifier orbId[4], stkId[4], folId[4], midId[2], nzId[2], xyId[6], wireId[50], stepId[16];
    WireSnap wires[50];
    int nWires = 0;
    OrbSnap orbSnap[4];
    StkSnap stkSnap[4];
    FolSnap folSnap[4];
    MidSnap midSnap[2];
    NzSnap nzSnap[2];
    XySnap xySnap[6];

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
        for (int k = 0; k < 16; ++k)
            stepId[k] = juce::Identifier ("s" + juce::String (k));
        for (int i = 0; i < 4; ++i)
        {
            orbId[i] = juce::Identifier ("orb" + juce::String (i));
            stkId[i] = juce::Identifier ("stk" + juce::String (i));
            folId[i] = juce::Identifier ("fol" + juce::String (i));
            ensureChild (orbId[i].toString(),
                         { { "wave", 0 }, { "morph", 0.0 }, { "rate_hz", 1.0 }, { "sync", false },
                           { "div", 2 }, { "phase", 0.0 }, { "freq_off", 0.0 }, { "balance", 0.0 },
                           { "trig", 0 } });
            auto o = tree.getChildWithName (orbId[i]);
            if (! o.getChildWithName ("steps").isValid())
            {
                juce::ValueTree st ("steps");
                for (int k = 0; k < 16; ++k)
                    st.setProperty ("s" + juce::String (k), 0.0, nullptr);
                st.setProperty ("glide", 0.0, nullptr);
                o.appendChild (st, nullptr);
            }
            ensureChild (stkId[i].toString(),
                         { { "a", 0.01 }, { "d", 0.1 }, { "s", 0.7 }, { "r", 0.2 },
                           { "ca", 0.0 }, { "cd", 0.0 }, { "cr", 0.0 },
                           { "trig", 0 }, { "thresh", -24.0 }, { "range", 0 }, { "retrig", true } });
            ensureChild (folId[i].toString(),
                         { { "mode", 0 }, { "atk", 10.0 }, { "rel", 100.0 }, { "thresh", -40.0 },
                           { "gain", 1.0 }, { "det_hz", 1000.0 }, { "det_width", 1.0 }, { "src", 0 } });
        }
        for (int i = 0; i < 2; ++i)
        {
            midId[i] = juce::Identifier ("mid" + juce::String (i));
            nzId[i] = juce::Identifier ("nz" + juce::String (i));
            ensureChild (midId[i].toString(), { { "kind", 0 }, { "cc", 1 } });
            ensureChild (nzId[i].toString(),
                         { { "kind", 0 }, { "rate_hz", 2.0 }, { "sync", false }, { "div", 2 }, { "smooth", 0.2 } });
        }
        for (int i = 0; i < 6; ++i)
        {
            xyId[i] = juce::Identifier ("xy" + juce::String (i));
            ensureChild (xyId[i].toString(), { { "x", 0.0 }, { "y", 0.0 }, { "unipolar", false }, { "slider", false } });
        }
        for (int i = 0; i < 50; ++i)
        {
            wireId[i] = juce::Identifier ("wire" + juce::String (i));
            ensureChild (wireId[i].toString(),
                         { { "src", i == 0 ? "orb:0" : "" }, { "dst", juce::String() },
                           { "depth", 0.0 }, { "on", false }, { "lag", 0.0 } });
        }
        pullFromTree();
    }

    static int parseSrc (const juce::String& src, int& idx)
    {
        if (src.startsWith ("orb:")) { idx = juce::jlimit (0, 3, src.getTrailingIntValue()); return kSrcOrb; }
        if (src.startsWith ("stk:")) { idx = juce::jlimit (0, 3, src.getTrailingIntValue()); return kSrcStk; }
        if (src.startsWith ("fol:")) { idx = juce::jlimit (0, 3, src.getTrailingIntValue()); return kSrcFol; }
        if (src.startsWith ("mid:")) { idx = juce::jlimit (0, 1, src.getTrailingIntValue()); return kSrcMid; }
        if (src.startsWith ("xy:"))  { idx = juce::jlimit (0, 5, src.getTrailingIntValue()); return kSrcXy; }
        if (src.startsWith ("nz:"))  { idx = juce::jlimit (0, 1, src.getTrailingIntValue()); return kSrcNz; }
        return -1;
    }

    static bool parseDst (const juce::String& dst, int& kind, int& idx)
    {
        idx = 0;
        if (dst == ParamID::glob_os || dst == ParamID::glob_linphase || dst == ParamID::glob_route
            || dst == ParamID::glob_bands || dst == ParamID::glob_listen)
            return false;
        if (dst == ParamID::glob_mix) { kind = kDstMix; return true; }
        if (dst == ParamID::glob_heat) { kind = kDstHeat; return true; }
        if (dst == ParamID::glob_spread) { kind = kDstSpread; return true; }
        if (dst == ParamID::glob_drive_db) { kind = kDstDrive; return true; }
        if (dst == ParamID::glob_blend) { kind = kDstBlend; return true; }
        if (dst == ParamID::glob_fb_amt) { kind = kDstFbAmt; return true; }
        if (dst == ParamID::glob_comp) { kind = kDstComp; return true; }
        if (dst == ParamID::glob_tone) { kind = kDstTone; return true; }
        if (dst == ParamID::glob_out_db) { kind = kDstOut; return true; }
        for (int i = 0; i < 6; ++i)
        {
            if (dst == ParamID::cDriveDb[i]) { kind = kDstCDrive; idx = i; return true; }
            if (dst == ParamID::cMix[i]) { kind = kDstCMix; idx = i; return true; }
            if (dst == ParamID::cBias[i]) { kind = kDstCBias; idx = i; return true; }
            if (dst == ParamID::cPan[i]) { kind = kDstCPan; idx = i; return true; }
            if (dst == ParamID::cDyn[i]) { kind = kDstCDyn; idx = i; return true; }
            if (dst == ParamID::cLevelDb[i]) { kind = kDstCLevel; idx = i; return true; }
            if (dst == ParamID::cFbAmt[i]) { kind = kDstCFbAmt; idx = i; return true; }
            if (dst == ParamID::cFiltHz[i]) { kind = kDstCFiltHz; idx = i; return true; }
            if (dst == ParamID::cFiltQ[i]) { kind = kDstCFiltQ; idx = i; return true; }
        }
        for (int i = 0; i < 5; ++i)
            if (dst == ParamID::xoverHz[i]) { kind = kDstXover; idx = i; return true; }
        return false;
    }

    void pullFromTree()
    {
        nWires = 0;
        for (int i = 0; i < 50; ++i)
        {
            auto w = tree.getChildWithName (wireId[i]);
            if (! (bool) w.getProperty ("on"))
                continue;
            const auto src = w.getProperty ("src").toString();
            const auto dst = w.getProperty ("dst").toString();
            if (src.isEmpty() || dst.isEmpty())
                continue;
            int srcIdx = 0, dstKind = 0, dstIdx = 0;
            const int srcKind = parseSrc (src, srcIdx);
            if (srcKind < 0 || ! parseDst (dst, dstKind, dstIdx))
                continue;
            auto& s = wires[nWires++];
            s.srcKind = srcKind;
            s.srcIdx = srcIdx;
            s.dstKind = dstKind;
            s.dstIdx = dstIdx;
            s.slot = i;
            s.depth = (float) w.getProperty ("depth");
            s.lag = (float) w.getProperty ("lag");
        }
        if (nWires <= 0)
            return;

        for (int i = 0; i < 4; ++i)
        {
            auto o = tree.getChildWithName (orbId[i]);
            orbSnap[i].wave = (int) o.getProperty ("wave");
            orbSnap[i].rateHz = (float) o.getProperty ("rate_hz");
            orbSnap[i].sync = (bool) o.getProperty ("sync");
            orbSnap[i].div = (int) o.getProperty ("div");
            auto steps = o.getChildWithName ("steps");
            orbSnap[i].glide = (float) steps.getProperty ("glide");
            for (int k = 0; k < 16; ++k)
                orbSnap[i].steps[k] = (float) steps.getProperty (stepId[k]);

            auto s = tree.getChildWithName (stkId[i]);
            stkSnap[i].trig = (int) s.getProperty ("trig");
            stkSnap[i].a = (float) s.getProperty ("a");
            stkSnap[i].d = (float) s.getProperty ("d");
            stkSnap[i].s = (float) s.getProperty ("s");
            stkSnap[i].r = (float) s.getProperty ("r");
            stkSnap[i].thresh = (float) s.getProperty ("thresh");
            stkSnap[i].retrig = (bool) s.getProperty ("retrig");

            auto f = tree.getChildWithName (folId[i]);
            folSnap[i].src = (int) f.getProperty ("src");
            folSnap[i].atk = (float) f.getProperty ("atk");
            folSnap[i].rel = (float) f.getProperty ("rel");
            folSnap[i].thresh = (float) f.getProperty ("thresh");
            folSnap[i].gain = (float) f.getProperty ("gain");
        }
        for (int i = 0; i < 2; ++i)
        {
            auto m = tree.getChildWithName (midId[i]);
            midSnap[i].kind = (int) m.getProperty ("kind");
            midSnap[i].cc = (int) m.getProperty ("cc");
            auto n = tree.getChildWithName (nzId[i]);
            nzSnap[i].kind = (int) n.getProperty ("kind");
            nzSnap[i].rateHz = (float) n.getProperty ("rate_hz");
            nzSnap[i].smooth = (float) n.getProperty ("smooth");
        }
        for (int i = 0; i < 6; ++i)
        {
            auto x = tree.getChildWithName (xyId[i]);
            xySnap[i].x = (float) x.getProperty ("x");
            xySnap[i].y = (float) x.getProperty ("y");
        }
    }

    static float wave (int kind, float p, float snh, const float steps[16], float glide)
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
            const float a = steps[i];
            const int j = (i + 1) & 15;
            const float b = steps[j];
            const float f = p * 16.f - (float) i;
            return a + (b - a) * (glide * f);
        }
        return std::sin (p * 6.2831853f);
    }

    float noiseTick (int i, float dt)
    {
        const auto& n = nzSnap[i];
        const int kind = n.kind;
        const float rate = juce::jlimit (0.01f, 40.f, n.rateHz);
        const float sm = juce::jlimit (0.f, 1.f, n.smooth);
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

    float sourceValue (int srcKind, int i, float dt, float bpm, float mainRms, float scRms, const float lensRms[6])
    {
        if (srcKind == kSrcOrb)
        {
            const auto& o = orbSnap[i];
            float rate = juce::jlimit (0.01f, 40.f, o.rateHz);
            if (o.sync)
            {
                static const float beats[] = { 0.125f, 0.25f, 0.5f, 1.f, 2.f, 4.f };
                const int div = juce::jlimit (0, 5, o.div);
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
            return wave (o.wave, orbPhase[i], orbSnh[i], o.steps, o.glide);
        }
        if (srcKind == kSrcStk)
        {
            const auto& s = stkSnap[i];
            const float th = fd1st::dbToLin (s.thresh);
            bool hit = false;
            if (s.trig == 0)
                hit = midiTrig;
            else if (s.trig == 1)
                hit = mainRms > th;
            else if (s.trig == 2)
                hit = scRms > th;
            else
                hit = lensRms[juce::jlimit (0, 5, s.trig - 3)] > th;
            const float A = juce::jmax (0.0001f, s.a);
            const float D = juce::jmax (0.001f, s.d);
            const float S = juce::jlimit (0.f, 1.f, s.s);
            const float R = juce::jmax (0.001f, s.r);
            if (hit && (stkStage[i] == 0 || s.retrig))
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
        if (srcKind == kSrcFol)
        {
            const auto& f = folSnap[i];
            float x = mainRms;
            if (f.src == 1)
                x = scRms;
            else if (f.src >= 2)
                x = lensRms[juce::jlimit (0, 5, f.src - 2)];
            const float atk = juce::jmax (0.0005f, f.atk * 0.001f);
            const float rel = juce::jmax (0.005f, f.rel * 0.001f);
            const float a = x > fol[i] ? (1.f - std::exp (-dt / atk)) : (1.f - std::exp (-dt / rel));
            fol[i] += (x - fol[i]) * a;
            const float th = fd1st::dbToLin (f.thresh);
            float y = juce::jlimit (0.f, 1.f, (fol[i] - th) / (1.f - th + 1.0e-8f));
            y *= f.gain;
            return juce::jlimit (-1.f, 1.f, y * 2.f - 1.f);
        }
        if (srcKind == kSrcMid)
        {
            const auto& m = midSnap[i];
            if (m.kind == 1)
                return midiVel * 2.f - 1.f;
            if (m.kind == 2)
                return midiNote * 2.f - 1.f;
            if (m.kind == 3)
                return midiBend;
            if (m.kind == 4)
                return midiPress * 2.f - 1.f;
            const int cc = juce::jlimit (0, 127, m.cc);
            return midiCC[cc] * 2.f - 1.f;
        }
        if (srcKind == kSrcXy)
        {
            xy[i][0] = xySnap[i].x;
            xy[i][1] = xySnap[i].y;
            return xy[i][0];
        }
        if (srcKind == kSrcNz)
            return noiseTick (i, dt);
        return 0.f;
    }

    void addDest (int kind, int idx, float v)
    {
        switch (kind)
        {
            case kDstMix: mods.glob_mix += v; break;
            case kDstHeat: mods.glob_heat += v; break;
            case kDstSpread: mods.glob_spread += v; break;
            case kDstDrive: mods.glob_drive += v; break;
            case kDstBlend: mods.glob_blend += v; break;
            case kDstFbAmt: mods.glob_fb_amt += v; break;
            case kDstComp: mods.glob_comp += v; break;
            case kDstTone: mods.glob_tone += v; break;
            case kDstOut: mods.glob_out += v; break;
            case kDstCDrive: mods.cDrive[idx] += v; break;
            case kDstCMix: mods.cMix[idx] += v; break;
            case kDstCBias: mods.cBias[idx] += v; break;
            case kDstCPan: mods.cPan[idx] += v; break;
            case kDstCDyn: mods.cDyn[idx] += v; break;
            case kDstCLevel: mods.cLevel[idx] += v; break;
            case kDstCFbAmt: mods.cFbAmt[idx] += v; break;
            case kDstCFiltHz: mods.cFiltHz[idx] += v; break;
            case kDstCFiltQ: mods.cFiltQ[idx] += v; break;
            case kDstXover: mods.xover[idx] += v; break;
            default: break;
        }
    }

    void tick (float dt, float bpm, float globMotion, float mainRms, float scRms, const float lensRms[6])
    {
        mods = {};
        if (nWires <= 0)
            return;
        for (int i = 0; i < nWires; ++i)
        {
            const auto& w = wires[i];
            float v = sourceValue (w.srcKind, w.srcIdx, dt, bpm, mainRms, scRms, lensRms);
            v *= w.depth * globMotion;
            if (w.lag > 0.5f)
            {
                const float a = 1.f - std::exp (-dt / (w.lag * 0.001f));
                lagZ[w.slot] += (v - lagZ[w.slot]) * a;
                v = lagZ[w.slot];
            }
            addDest (w.dstKind, w.dstIdx, v);
        }
    }

    // Back-compat for the old one-wire Mix path.
    bool wireOn = false;
    float wireDepth = 0;
    juce::String wireDst;
    float orbitSample (float dt)
    {
        float z[6] {};
        pullFromTree();
        tick (dt, 120.f, 1.f, 0, 0, z);
        auto w = tree.getChildWithName (wireId[0]);
        wireOn = (bool) w.getProperty ("on");
        wireDepth = (float) w.getProperty ("depth");
        wireDst = w.getProperty ("dst").toString();
        return sourceValue (kSrcOrb, 0, dt, 120.f, 0, 0, z);
    }
};

} // namespace fd1st
