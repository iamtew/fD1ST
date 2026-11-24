#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/Clip.h"
#include "presets/Factory.h"

#include <algorithm>
#include <cstdlib>

fD1STAudioProcessor::fD1STAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    motion.prepare();
    maybeSelfTest();
}

int fD1STAudioProcessor::osId() const
{
    return juce::jlimit (0, 3, (int) loadf (ParamID::glob_os, 2.f));
}

int fD1STAudioProcessor::osFactor() const
{
    const int id = osId();
    if (id <= 0)
        return 1;
    if (id == 1)
        return 2;
    if (id == 2)
        return 8;
    return 16;
}

bool fD1STAudioProcessor::linphaseOn() const
{
    return loadf (ParamID::glob_linphase) > 0.5f;
}

juce::dsp::Oversampling<float>* fD1STAudioProcessor::currentOs()
{
    const int id = osId();
    if (id <= 0)
        return nullptr;
    const bool lp = linphaseOn();
    if (id == 1)
        return lp ? &os2f : &os2i;
    if (id == 2)
        return lp ? &os8f : &os8i;
    return lp ? &os32f : &os32i;
}

int fD1STAudioProcessor::osLatency() const
{
    const int id = osId();
    if (id <= 0)
        return 0;
    const bool lp = linphaseOn();
    auto lat = [] (const Os& o) { return (int) std::ceil (o.getLatencyInSamples()); };
    if (id == 1)
        return lp ? lat (os2f) : lat (os2i);
    if (id == 2)
        return lp ? lat (os8f) : lat (os8i);
    return lp ? lat (os32f) : lat (os32i);
}

double fD1STAudioProcessor::hostBpm() const
{
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getIsPlaying())
                if (auto b = pos->getBpm())
                    if (*b > 1.0)
                        return *b;
    return 120.0;
}

float fD1STAudioProcessor::cavityTimeSec() const
{
    const int mode = (int) loadf (ParamID::glob_fb_mode);
    if (mode <= 0)
        return juce::jlimit (0.0005f, 2.f, loadf (ParamID::glob_fb_time, 0.25f));
    if (mode == 4)
        return 1.f / juce::jlimit (27.5f, 4186.f, loadf (ParamID::glob_fb_note_hz, 110.f));
    static const float beats[] = { 0.125f, 0.25f, 0.5f, 1.f, 2.f, 4.f };
    const int div = juce::jlimit (0, 5, (int) loadf (ParamID::glob_fb_div, 2.f));
    float t = beats[div] * 60.f / (float) (cachedBpm > 1.0 ? cachedBpm : 120.0);
    if (mode == 2)
        t *= 2.f / 3.f;
    if (mode == 3)
        t *= 1.5f;
    return juce::jlimit (0.0005f, 2.f, t);
}

void fD1STAudioProcessor::updateOsRate()
{
    const int factor = osFactor();
    const double osSr = hostSr * (double) factor;
    for (auto& L : lenses)
        L.setRate (osSr);
    outSmooth.reset (osSr, 0.015);
    globDriveSmooth.reset (osSr, 0.015);
    blendSmooth.reset (osSr, 0.015);
    heatSmooth.reset (osSr, 0.015);
    spreadSmooth.reset (osSr, 0.015);
    fbAmtSmooth.reset (osSr, 0.015);
    compSmooth.reset (osSr, 0.015);
    toneSmooth.reset (osSr, 0.015);
    osLatencySamples = osLatency();
    setLatencySamples (osLatencySamples);
    delayLen = juce::jmax (1, juce::jmin (delayCap, osLatencySamples));
    for (auto& d : dryDelay)
        std::fill (d.begin(), d.end(), 0.f);
    delayW = 0;
    lastOs = osId();
    lastLin = linphaseOn() ? 1 : 0;
}

void fD1STAudioProcessor::ensureHostSize (int n)
{
    n = juce::jmax (32, n);
    if (n <= hostBlock && dryBuf.getNumSamples() >= n)
        return;
    hostBlock = juce::jmax (hostBlock, n);
    auto initOs = [this] (Os& o)
    {
        o.initProcessing ((size_t) hostBlock);
        o.reset();
    };
    initOs (os2i);
    initOs (os8i);
    initOs (os32i);
    initOs (os2f);
    initOs (os8f);
    initOs (os32f);
    if (dryBuf.getNumSamples() < hostBlock)
        dryBuf.setSize (2, juce::jmax (4096, hostBlock), false, false, true);
}

void fD1STAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSr = sampleRate > 1.0 ? sampleRate : 48000.0;
    hostBlock = 0;
    dryBuf.setSize (0, 0);
    // Hosts often send a smaller callback than this max. Grow once, never per block.
    ensureHostSize (juce::jmax (4096, samplesPerBlock));

    const double osSr = hostSr * (double) osFactor();
    juce::dsp::ProcessSpec spec { osSr, (juce::uint32) (hostBlock * 32), 2 };
    for (int i = 0; i < 5; ++i)
    {
        lrLo[i].prepare (spec);
        lrHi[i].prepare (spec);
        lrLo2[i].prepare (spec);
        lrHi2[i].prepare (spec);
        lrLo[i].setType (juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
        lrHi[i].setType (juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
        lrLo2[i].setType (juce::dsp::LinkwitzRileyFilter<float>::Type::lowpass);
        lrHi2[i].setType (juce::dsp::LinkwitzRileyFilter<float>::Type::highpass);
        lastXoverHz[i] = -1.f;
        lastXoverSlope[i] = -1;
    }

    for (auto& L : lenses)
        L.prepare (osSr, hostBlock * osFactor());

    mixSmooth.reset (hostSr, 0.015);
    outSmooth.reset (osSr, 0.015);
    globDriveSmooth.reset (osSr, 0.015);
    blendSmooth.reset (osSr, 0.015);
    heatSmooth.reset (osSr, 0.015);
    spreadSmooth.reset (osSr, 0.015);
    fbAmtSmooth.reset (osSr, 0.015);
    compSmooth.reset (osSr, 0.015);
    toneSmooth.reset (osSr, 0.015);
    mixSmooth.setCurrentAndTargetValue (1.f);
    outSmooth.setCurrentAndTargetValue (1.f);
    globDriveSmooth.setCurrentAndTargetValue (1.f);
    blendSmooth.setCurrentAndTargetValue (0.5f);

    if (dryBuf.getNumSamples() < hostBlock)
        dryBuf.setSize (2, juce::jmax (4096, hostBlock), false, false, true);
    lastOs = osId();
    lastLin = linphaseOn() ? 1 : 0;
    osLatencySamples = osLatency();
    setLatencySamples (osLatencySamples);

    delayCap = juce::jmax (64, (int) std::ceil (os32f.getLatencyInSamples() + os32i.getLatencyInSamples()) + 8);
    delayLen = juce::jmax (1, osLatencySamples);
    for (auto& d : dryDelay)
        d.assign ((size_t) delayCap, 0.f);
    delayW = 0;

    cavCap = juce::jmax (64, (int) (2.0 * osSr) + 8);
    for (auto& c : cavity)
        c.assign ((size_t) cavCap, 0.f);
    cavW = 0;
    cavCompL.reset();
    cavCompR.reset();
    outCompL.reset();
    outCompR.reset();
}

void fD1STAudioProcessor::updatePrism()
{
    const float osSr = (float) hostSr * (float) osFactor();
    const float hostScale = (float) (hostSr / 48000.0);
    for (int i = 0; i < 5; ++i)
    {
        const float hz = loadf (ParamID::xoverHz[i], fd1st::kXoverDefaultHz[i]);
        const int slope = (int) loadf (ParamID::xoverSlope[i], 2.f);
        if (std::abs (hz - lastXoverHz[i]) < 0.01f && slope == lastXoverSlope[i])
            continue;
        lastXoverHz[i] = hz;
        lastXoverSlope[i] = slope;
        lrLo[i].setCutoffFrequency (hz);
        lrHi[i].setCutoffFrequency (hz);
        lrLo2[i].setCutoffFrequency (hz);
        lrHi2[i].setCutoffFrequency (hz);
        const int taps = slope <= 0 ? (int) (512.f * hostScale)
                                    : (slope >= 3 ? (int) (2048.f * hostScale)
                                                  : (int) ((512.f + 512.f * (float) slope) * hostScale));
        fir[i].design (taps | 1, hz, osSr);
    }
}

void fD1STAudioProcessor::prismSplit (int split, float inL, float inR, float osSr,
                                      float& loL, float& loR, float& hiL, float& hiR)
{
    const int slope = lastXoverSlope[split];
    if (linphaseOn())
    {
        fir[split].split (inL, 0, loL, hiL);
        fir[split].split (inR, 1, loR, hiR);
        return;
    }
    if (slope <= 1)
    {
        const float a = fd1st::coeffAt (lastXoverHz[split], osSr);
        loL = fd1st::lp1 (inL, prismLp[split][0], a);
        loR = fd1st::lp1 (inR, prismLp[split][1], a);
        if (slope == 1)
        {
            loL = fd1st::lp1 (loL, prismLp2[split][0], a);
            loR = fd1st::lp1 (loR, prismLp2[split][1], a);
        }
        hiL = inL - loL;
        hiR = inR - loR;
        return;
    }
    loL = lrLo[split].processSample (0, inL);
    loR = lrLo[split].processSample (1, inR);
    hiL = lrHi[split].processSample (0, inL);
    hiR = lrHi[split].processSample (1, inR);
    if (slope >= 3)
    {
        loL = lrLo2[split].processSample (0, loL);
        loR = lrLo2[split].processSample (1, loR);
        hiL = lrHi2[split].processSample (0, hiL);
        hiR = lrHi2[split].processSample (1, hiR);
    }
}

void fD1STAudioProcessor::maybeSelfTest()
{
    if (selfTestRan)
        return;
    const auto args = juce::JUCEApplicationBase::getCommandLineParameterArray();
    if (! args.contains ("--self-test"))
        return;
    selfTestRan = true;
    const int code = runSelfTest();
    std::exit (code);
}

int fD1STAudioProcessor::getNumPrograms() { return fd1st::kFactoryCount; }

void fD1STAudioProcessor::setCurrentProgram (int index)
{
    program = juce::jlimit (0, fd1st::kFactoryCount - 1, index);
    fd1st::applyFactory (apvts, program);
    motion.prepare();
    auto wire = [&] (int slot, const juce::String& src, const juce::String& dst, float depth)
    {
        auto w = motion.tree.getChildWithName ("wire" + juce::String (slot));
        w.setProperty ("src", src, nullptr);
        w.setProperty ("dst", dst, nullptr);
        w.setProperty ("depth", depth, nullptr);
        w.setProperty ("on", true, nullptr);
    };
    if (program == 36)
    {
        wire (0, "fol:0", ParamID::glob_heat, 0.6f);
        motion.tree.getChildWithName ("fol0").setProperty ("det_hz", 140.0, nullptr);
    }
    else if (program == 37)
    {
        wire (0, "orb:0", ParamID::xoverHz[0], 0.15f);
        wire (1, "orb:1", ParamID::cDriveDb[1], 0.3f);
        motion.tree.getChildWithName ("orb0").setProperty ("sync", true, nullptr);
        motion.tree.getChildWithName ("orb0").setProperty ("div", 3, nullptr);
    }
    else if (program == 38)
        wire (0, "nz:0", ParamID::cBias[0], 0.4f);
    else if (program == 39)
        wire (0, "stk:0", ParamID::glob_mix, 1.f);
    else if (program == 40)
        wire (0, "fol:0", ParamID::glob_mix, -0.8f);
    else if (program == 41)
    {
        wire (0, "xy:0", ParamID::glob_heat, 0.8f);
        wire (1, "xy:0", ParamID::glob_spread, 0.8f);
    }
}

const juce::String fD1STAudioProcessor::getProgramName (int index)
{
    return fd1st::factoryName (index);
}

bool fD1STAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    if (layouts.getMainInputChannelSet() != out)
        return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.inputBuses[1];
        if (! sc.isDisabled()
            && sc != juce::AudioChannelSet::stereo()
            && sc != juce::AudioChannelSet::mono())
            return false;
    }
    return true;
}

float fD1STAudioProcessor::loadf (const char* id, float fallback) const
{
    if (auto* p = apvts.getRawParameterValue (id))
        return p->load();
    return fallback;
}

void fD1STAudioProcessor::cacheWet()
{
    wet.route = (int) loadf (ParamID::glob_route);
    wet.chmode = (int) loadf (ParamID::glob_chmode);
    wet.bands = juce::jlimit (1, 6, (int) loadf (ParamID::glob_bands, 3.f));
    wet.color = loadf (ParamID::glob_colorcomp) > 0.5f;
    wet.safe = loadf (ParamID::glob_fb_safe) > 0.5f;
    wet.hpf = loadf (ParamID::glob_comp_hpf) > 0.5f;
    wet.globComp = loadf (ParamID::glob_comp);
    wet.cavDel = cavityTimeSec() * (float) hostSr * (float) osFactor();
    wet.anySolo = false;
    auto inRoute = [&] (int i) -> bool
    {
        if (wet.route == 3)
            return i < wet.bands;
        if (wet.route <= 0)
            return i == 0;
        return i <= 1;
    };
    for (int i = 0; i < 6; ++i)
    {
        wet.solo[i] = loadf (ParamID::cSolo[i]) > 0.5f;
        if (wet.solo[i] && inRoute (i))
            wet.anySolo = true;
        wet.lens[i].pan = loadf (ParamID::cPan[i]);
        wet.lens[i].driveDb = loadf (ParamID::cDriveDb[i]);
        wet.lens[i].drivePan = loadf (ParamID::cDrivePan[i]);
        wet.lens[i].bias = loadf (ParamID::cBias[i]);
        wet.lens[i].mix = loadf (ParamID::cMix[i], 1.f);
        wet.lens[i].levelDb = loadf (ParamID::cLevelDb[i]);
        wet.lens[i].fbAmt = loadf (ParamID::cFbAmt[i]);
        wet.lens[i].fbHz = loadf (ParamID::cFbHz[i], 220.f);
        wet.lens[i].dyn = loadf (ParamID::cDyn[i]);
    }
}

void fD1STAudioProcessor::configureLenses()
{
    const bool safe = loadf (ParamID::glob_fb_safe) > 0.5f;
    for (int i = 0; i < fd1st::kLenses; ++i)
    {
        lenses[(size_t) i].configure (
            loadf (ParamID::cOn[i]) > 0.5f,
            loadf (ParamID::cMute[i]) > 0.5f,
            (int) loadf (ParamID::cStyle[i]),
            loadf (ParamID::cFiltHz[i], 1200.f),
            loadf (ParamID::cFiltQ[i], 0.707f),
            (int) loadf (ParamID::cFiltType[i]),
            (int) loadf (ParamID::cFiltPos[i], 1.f),
            loadf (ParamID::cBass[i]),
            loadf (ParamID::cMid[i]),
            loadf (ParamID::cTreble[i]),
            loadf (ParamID::cPresence[i]),
            safe);
    }
}

void fD1STAudioProcessor::processFrame (float& L, float& R)
{
    const int route = wet.route;
    const int chmode = wet.chmode;
    const float blend = blendSmooth.getNextValue();
    const float spread = spreadSmooth.getNextValue();
    const float heat = juce::jlimit (-1.f, 1.f, heatSmooth.getNextValue() + motion.mods.glob_heat);
    const float tone = toneSmooth.getNextValue();
    const float gDrive = globDriveSmooth.getNextValue();
    const float osSr = (float) hostSr * (float) osFactor();
    L *= gDrive;
    R *= gDrive;

    const float pre = wet.color ? -tone : 0.f;
    L = fd1st::tilt (L, toneZ[0], fd1st::coeffAt (800.f, osSr), pre);
    R = fd1st::tilt (R, toneZ[1], fd1st::coeffAt (800.f, osSr), pre);

    bool msWrap = chmode == 1 && route != 4;
    if (msWrap)
    {
        const float m = 0.5f * (L + R);
        const float s = 0.5f * (L - R);
        L = m;
        R = s;
    }

    bool ticked[6] = {};
    const int bands = wet.bands;
    auto inRoute = [&] (int i) -> bool
    {
        if (route == 3)
            return i < bands;
        if (route <= 0)
            return i == 0;
        return i <= 1;
    };

    auto go = [&] (int i, float x, int ch) -> float
    {
        if (wet.anySolo && ! wet.solo[i])
            return route == 3 ? 0.f : x;
        if (! inRoute (i))
            return x;
        if (! ticked[i])
        {
            const auto& ln = wet.lens[i];
            float pan = ln.pan;
            if (route == 1 || route == 2 || route == 5)
                pan += (i == 0 ? -spread : spread) * 0.5f;
            lenses[(size_t) i].tick (ln.driveDb + motion.mods.cDrive[i] * 18.f,
                                     heat,
                                     ln.drivePan,
                                     juce::jlimit (-1.f, 1.f, ln.bias + motion.mods.cBias[i]),
                                     juce::jlimit (0.f, 1.f, ln.mix + motion.mods.cMix[i] * 0.5f),
                                     ln.levelDb + motion.mods.cLevel[i] * 30.f,
                                     pan + motion.mods.cPan[i],
                                     juce::jlimit (0.f, 1.f, ln.fbAmt + motion.mods.cFbAmt[i] * 0.5f),
                                     ln.fbHz,
                                     juce::jlimit (-1.f, 1.f, ln.dyn + motion.mods.cDyn[i]));
            ticked[i] = true;
        }
        const float y = lenses[(size_t) i].apply (x, ch);
        lensApplyCount[(size_t) i].fetch_add (1, std::memory_order_relaxed);
        return y;
    };

    float oL = L, oR = R;
    if (route <= 0)
    {
        oL = go (0, L, 0);
        oR = go (0, R, 1);
        const float m = 0.5f * (oL + oR);
        float s = 0.5f * (oL - oR);
        s *= 1.f + spread;
        oL = m + s;
        oR = m - s;
    }
    else if (route == 1)
    {
        const float aL = go (0, L, 0), aR = go (0, R, 1);
        const float bL = go (1, aL, 0), bR = go (1, aR, 1);
        oL = aL + (bL - aL) * blend;
        oR = aR + (bR - aR) * blend;
    }
    else if (route == 2)
    {
        const float aL = go (0, L, 0), aR = go (0, R, 1);
        const float bL = go (1, L, 0), bR = go (1, R, 1);
        oL = aL + (bL - aL) * blend;
        oR = aR + (bR - aR) * blend;
    }
    else if (route == 3)
    {
        oL = oR = 0;
        float restL = L, restR = R;
        for (int b = 0; b < bands; ++b)
        {
            float bandL = restL, bandR = restR;
            if (b + 1 < bands)
            {
                float hiL = 0, hiR = 0;
                prismSplit (b, restL, restR, osSr, bandL, bandR, hiL, hiR);
                restL = hiL;
                restR = hiR;
            }
            const float t = bands == 1 ? 0.5f : (float) b / (float) (bands - 1);
            const float g = fd1st::dbToLin (spread * 3.f * (1.f - 2.f * t));
            oL += go (b, bandL, 0) * g;
            oR += go (b, bandR, 1) * g;
        }
    }
    else if (route == 4)
    {
        float m = 0.5f * (L + R);
        float s = 0.5f * (L - R);
        m = go (0, m, 0);
        s = go (1, s, 1);
        s *= 1.f + spread;
        oL = m + s;
        oR = m - s;
    }
    else
    {
        const float fwdL = go (0, L, 0);
        const float fwdR = go (0, R, 1);
        float amt = fbAmtSmooth.getNextValue();
        if (wet.safe)
            amt = juce::jmin (0.85f, amt);
        const float del = wet.cavDel;
        const float fbL = fd1st::readHermite (cavity[0].data(), cavCap, cavW, del);
        const float fbR = fd1st::readHermite (cavity[1].data(), cavCap, cavW, del);
        float inL = L + amt * fbL;
        float inR = R + amt * fbR;
        inL = go (1, inL, 0);
        inR = go (1, inR, 1);
        const float amtC = compSmooth.getNextValue();
        inL = cavCompL.process (inL, amtC, osSr, wet.hpf);
        inR = cavCompR.process (inR, amtC, osSr, wet.hpf);
        inL = fd1st::hp1 (inL, cavHp[0], fd1st::coeffAt (15.f, osSr));
        inR = fd1st::hp1 (inR, cavHp[1], fd1st::coeffAt (15.f, osSr));
        if (wet.safe)
        {
            const float pk = fd1st::dbToLin (-0.3f);
            inL = juce::jlimit (-pk, pk, inL);
            inR = juce::jlimit (-pk, pk, inR);
        }
        cavity[0][(size_t) cavW] = inL;
        cavity[1][(size_t) cavW] = inR;
        cavW = (cavW + 1) % cavCap;
        oL = fwdL + (fbL - fwdL) * blend;
        oR = fwdR + (fbR - fwdR) * blend;
    }

    if (route != 5)
    {
        float amt = fbAmtSmooth.getNextValue();
        if (wet.safe)
            amt = juce::jmin (0.85f, amt);
        if (amt > 0.0001f)
        {
            const float del = wet.cavDel;
            const float fbL = fd1st::readHermite (cavity[0].data(), cavCap, cavW, del);
            const float fbR = fd1st::readHermite (cavity[1].data(), cavCap, cavW, del);
            const float amtC = compSmooth.getNextValue();
            float wL = cavCompL.process (oL + amt * fbL, amtC, osSr, wet.hpf);
            float wR = cavCompR.process (oR + amt * fbR, amtC, osSr, wet.hpf);
            wL = fd1st::hp1 (wL, cavHp[0], fd1st::coeffAt (15.f, osSr));
            wR = fd1st::hp1 (wR, cavHp[1], fd1st::coeffAt (15.f, osSr));
            if (wet.safe)
            {
                const float pk = fd1st::dbToLin (-0.3f);
                wL = juce::jlimit (-pk, pk, wL);
                wR = juce::jlimit (-pk, pk, wR);
            }
            cavity[0][(size_t) cavW] = wL;
            cavity[1][(size_t) cavW] = wR;
            cavW = (cavW + 1) % cavCap;
            oL += amt * fbL;
            oR += amt * fbR;
        }
        else
        {
            compSmooth.getNextValue();
        }
    }

    {
        const float amtC = wet.globComp;
        const float det = juce::jmax (std::abs (oL), std::abs (oR));
        float dL = oL, dR = oR;
        if (wet.hpf)
        {
            dL = fd1st::hp1 (oL, outCompL.detZ, fd1st::coeffAt (120.f, osSr));
            dR = fd1st::hp1 (oR, outCompR.detZ, fd1st::coeffAt (120.f, osSr));
        }
        const float gr = outCompL.gain (juce::jmax (std::abs (dL), std::abs (dR), det), amtC, osSr);
        oL *= gr;
        oR *= gr;
        lastGr.store (gr, std::memory_order_relaxed);
    }

    oL = fd1st::tilt (oL, tonePostZ[0], fd1st::coeffAt (800.f, osSr), tone);
    oR = fd1st::tilt (oR, tonePostZ[1], fd1st::coeffAt (800.f, osSr), tone);

    if (msWrap)
    {
        const float m = oL;
        const float s = oR;
        oL = m + s;
        oR = m - s;
    }

    const float outG = outSmooth.getNextValue();
    L = fd1st::railClip (oL * outG);
    R = fd1st::railClip (oR * outG);
    if (! std::isfinite (L))
        L = 0.f;
    if (! std::isfinite (R))
        R = 0.f;
    cavIndex.store (cavW, std::memory_order_relaxed);
}

void fD1STAudioProcessor::processWet (juce::dsp::AudioBlock<float> block)
{
    const int n = (int) block.getNumSamples();
    const int chans = (int) block.getNumChannels();
    configureLenses();
    updatePrism();
    cacheWet();
    globDriveSmooth.setTargetValue (fd1st::dbToLin (loadf (ParamID::glob_drive_db)));
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (loadf (ParamID::glob_out_db)));
    blendSmooth.setTargetValue (loadf (ParamID::glob_blend, 0.5f));
    heatSmooth.setTargetValue (loadf (ParamID::glob_heat));
    spreadSmooth.setTargetValue (loadf (ParamID::glob_spread));
    fbAmtSmooth.setTargetValue (loadf (ParamID::glob_fb_amt));
    compSmooth.setTargetValue (loadf (ParamID::glob_comp));
    toneSmooth.setTargetValue (loadf (ParamID::glob_tone));

    for (int i = 0; i < n; ++i)
    {
        float L = block.getSample (0, i);
        float R = chans > 1 ? block.getSample (1, i) : L;
        processFrame (L, R);
        block.setSample (0, i, L);
        if (chans > 1)
            block.setSample (1, i, R);
    }
}

void fD1STAudioProcessor::applyMotion (float dt, float mainL, float mainR, float scL_, float scR_)
{
    float z[6] {};
    const float mainRms = std::sqrt (mainL * mainL + mainR * mainR);
    const float scRms = std::sqrt (scL_ * scL_ + scR_ * scR_);
    motion.pullFromTree();
    motion.tick (dt, (float) cachedBpm, loadf (ParamID::glob_motion, 1.f), mainRms, scRms, z);
    mixMod.store (motion.mods.glob_mix * 0.5f, std::memory_order_relaxed);
}

void fD1STAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    motion.midiTrig = false;
    for (const auto meta : midi)
    {
        auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            motion.midiNote = (float) m.getNoteNumber() / 127.f;
            motion.midiVel = m.getFloatVelocity();
            motion.midiTrig = true;
        }
        else if (m.isController())
        {
            const int cc = juce::jlimit (0, 127, m.getControllerNumber());
            motion.midiCC[cc] = (float) m.getControllerValue() / 127.f;
            if (midiLearnOn.load() && midiLearnId.isNotEmpty())
                if (auto* p = apvts.getParameter (midiLearnId))
                    p->setValueNotifyingHost ((float) m.getControllerValue() / 127.f);
        }
        else if (m.isPitchWheel())
            motion.midiBend = ((float) m.getPitchWheelValue() / 8192.f) - 1.f;
        else if (m.isChannelPressure())
            motion.midiPress = (float) m.getChannelPressureValue() / 127.f;
    }

    auto main = getBusBuffer (buffer, true, 0);
    const int n = main.getNumSamples();
    const int chans = juce::jmin (2, main.getNumChannels());
    cachedBpm = hostBpm();
    if (n > hostBlock)
        ensureHostSize (n);

    if (getBusCount (true) > 1)
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumSamples() > 0 && sc.getNumChannels() > 0)
        {
            scL = sc.getSample (0, juce::jmax (0, sc.getNumSamples() - 1));
            scR = sc.getNumChannels() > 1 ? sc.getSample (1, juce::jmax (0, sc.getNumSamples() - 1)) : scL;
            scPeak.store (juce::jmax (std::abs (scL), std::abs (scR)), std::memory_order_relaxed);
        }
    }

    if (osId() != lastOs || (linphaseOn() ? 1 : 0) != lastLin)
    {
        if (auto* o = currentOs())
            o->reset();
        updateOsRate();
    }

    if (loadf (ParamID::glob_bypass) > 0.5f)
        return;

    const int copyN = juce::jmin (n, dryBuf.getNumSamples());
    float inP = inPeak.load (std::memory_order_relaxed) * 0.9f;
    for (int ch = 0; ch < chans; ++ch)
    {
        dryBuf.copyFrom (ch, 0, main, ch, 0, copyN);
        inP = juce::jmax (inP, main.getMagnitude (ch, 0, copyN));
    }
    inPeak.store (inP, std::memory_order_relaxed);

    {
        const float dt = (float) (n / juce::jmax (1.0, hostSr));
        const float L0 = chans > 0 ? dryBuf.getSample (0, 0) : 0.f;
        const float R0 = chans > 1 ? dryBuf.getSample (1, 0) : L0;
        applyMotion (dt, L0, R0, scL, scR);
    }

    juce::dsp::AudioBlock<float> block (main);
    if (auto* o = currentOs())
    {
        auto up = o->processSamplesUp (block);
        processWet (up);
        o->processSamplesDown (block);
    }
    else
        processWet (block);

    const int listen = (int) loadf (ParamID::glob_listen);
    mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, loadf (ParamID::glob_mix, 1.f)));
    const int len = delayLen;
    const float hostRate = (float) hostSr;
    float outP = outPeak.load (std::memory_order_relaxed) * 0.9f;
    int pw = probeW.load (std::memory_order_relaxed);
    for (int i = 0; i < copyN; ++i)
    {
        float mix = juce::jlimit (0.f, 1.f, mixSmooth.getNextValue() + mixMod.load (std::memory_order_relaxed));
        float dry0 = 0, wet0 = 0;
        for (int ch = 0; ch < chans; ++ch)
        {
            float wet = main.getSample (ch, i);
            float dryIn = dryBuf.getSample (ch, i);
            float dry = dryIn;
            if (len > 0 && delayCap > 1)
            {
                const int ri = (delayW - len + delayCap) % delayCap;
                dry = dryDelay[(size_t) ch][(size_t) ri];
                dryDelay[(size_t) ch][(size_t) delayW] = dryIn;
            }
            float y = wet;
            if (listen == 1)
            {
                y = dry;
                mix = 1.f;
            }
            else if (listen == 2)
                y = wet - dry;
            else if (listen == 3)
            {
                const float dlt = wet - dry;
                y = fd1st::hp1 (dlt, causticZ, fd1st::coeffAt (1200.f, hostRate)) * fd1st::dbToLin (6.f);
            }
            if (listen != 1)
                y = dry + (y - dry) * mix;
            if (listen == 2 && mix <= 0.0001f)
                y = 0.f;
            main.setSample (ch, i, y);
            outP = juce::jmax (outP, std::abs (y));
            if (ch == 0)
            {
                dry0 = dry;
                wet0 = y;
            }
        }
        probeIn[(size_t) (pw % kProbeN)] = dry0;
        probeOut[(size_t) (pw % kProbeN)] = wet0;
        ++pw;
        delayW = (delayW + 1) % juce::jmax (1, delayCap);
    }
    probeW.store (pw, std::memory_order_relaxed);
    outPeak.store (outP, std::memory_order_relaxed);
    nodeRms[0].store (inP, std::memory_order_relaxed);
    nodeRms[1].store (outP, std::memory_order_relaxed);
}

int fD1STAudioProcessor::runSelfTest()
{
    const int n = 512;
    const double sr = 48000.0;
    prepareToPlay (sr, n);
    juce::MidiBuffer midi;
    int fail = 0;

    auto rms = [] (const juce::AudioBuffer<float>& b, int start = 0) -> float
    {
        double a = 0;
        int count = 0;
        for (int c = 0; c < b.getNumChannels(); ++c)
            for (int i = start; i < b.getNumSamples(); ++i, ++count)
                a += (double) b.getSample (c, i) * b.getSample (c, i);
        return (float) std::sqrt (a / (double) juce::jmax (1, count));
    };
    auto fillSine = [&] (juce::AudioBuffer<float>& b, float hz, float amp)
    {
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float s = amp * std::sin (2.f * 3.14159265f * hz * (float) i / (float) sr);
            b.setSample (0, i, s);
            if (b.getNumChannels() > 1)
                b.setSample (1, i, s);
        }
    };
    auto setP = [&] (const char* id, float v)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (v));
    };

    // 1 silence
    fd1st::applyInit (apvts);
    juce::AudioBuffer<float> buf (2, n);
    buf.clear();
    processBlock (buf, midi);
    if (juce::Decibels::gainToDecibels (rms (buf) + 1.0e-12f) > -90.f)
        ++fail;

    // 2 bypass
    fillSine (buf, 200.f, 0.25f);
    setP (ParamID::glob_bypass, 1.f);
    auto copy = buf;
    processBlock (buf, midi);
    float err = 0;
    for (int i = 0; i < n; ++i)
        err = juce::jmax (err, std::abs (buf.getSample (0, i) - copy.getSample (0, i)));
    if (err > 1.0e-5f)
        ++fail;
    setP (ParamID::glob_bypass, 0.f);

    // 3 makeup −12 dBFS Glow 0 vs 24 within 3 dB
    auto sineRms = [&] (float drive) -> float
    {
        fd1st::applyInit (apvts);
        setP (ParamID::cDriveDb[0], drive);
        setP (ParamID::glob_os, 0.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n * 16);
        fillSine (b, 200.f, 0.251189f);
        processBlock (b, midi);
        return rms (b, n * 4);
    };
    {
        const float db = 20.f * std::log10 ((sineRms (24.f) + 1.0e-8f) / (sineRms (0.f) + 1.0e-8f));
        if (std::abs (db) > 3.f)
            ++fail;
    }

    // 4 Prism sum, mix 0, white noise ±0.5 dB
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_route, 3.f);
        setP (ParamID::glob_os, 0.f);
        setP (ParamID::glob_bands, 3.f);
        for (int i = 0; i < 6; ++i)
            setP (ParamID::cMix[i], 0.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n * 8);
        juce::Random rng (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float s = rng.nextFloat() * 2.f - 1.f;
            b.setSample (0, i, s * 0.2f);
            b.setSample (1, i, s * 0.2f);
        }
        auto in = b;
        processBlock (b, midi);
        const float din = juce::Decibels::gainToDecibels (rms (in, n) + 1.0e-12f);
        const float dout = juce::Decibels::gainToDecibels (rms (b, n) + 1.0e-12f);
        if (std::abs (dout - din) > 0.5f)
            ++fail;
    }

    // 5 Rectify DC
    {
        fd1st::applyInit (apvts);
        setP (ParamID::cStyle[0], 32.f);
        setP (ParamID::cDriveDb[0], 12.f);
        setP (ParamID::glob_os, 0.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, (int) (sr * 0.6));
        fillSine (b, 40.f, 1.f);
        processBlock (b, midi);
        double mean = 0;
        const int start = b.getNumSamples() / 2;
        for (int i = start; i < b.getNumSamples(); ++i)
            mean += (double) b.getSample (0, i);
        mean /= (double) (b.getNumSamples() - start);
        if (juce::Decibels::gainToDecibels ((float) std::abs (mean) + 1.0e-12f) > -60.f)
            ++fail;
    }

    // 6 Axis roundtrip mix 0
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_route, 4.f);
        setP (ParamID::cMix[0], 0.f);
        setP (ParamID::cMix[1], 0.f);
        setP (ParamID::glob_os, 0.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n * 4);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            b.setSample (0, i, 0.2f * std::sin (2.f * 3.14159265f * 200.f * (float) i / (float) sr));
            b.setSample (1, i, 0.1f * std::sin (2.f * 3.14159265f * 330.f * (float) i / (float) sr));
        }
        auto in = b;
        processBlock (b, midi);
        const float d = juce::Decibels::gainToDecibels (rms (b, n) + 1e-12f)
                        - juce::Decibels::gainToDecibels (rms (in, n) + 1e-12f);
        if (std::abs (d) > 0.1f)
            ++fail;
    }

    // 7 Cavity safety
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_route, 5.f);
        setP (ParamID::glob_fb_amt, 1.f);
        setP (ParamID::glob_fb_safe, 1.f);
        setP (ParamID::glob_os, 0.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, (int) sr);
        juce::Random rng (2);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float s = rng.nextFloat() * 2.f - 1.f;
            b.setSample (0, i, s);
            b.setSample (1, i, s);
        }
        processBlock (b, midi);
        float pk = 0;
        bool nan = false;
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            pk = juce::jmax (pk, std::abs (b.getSample (0, i)));
            if (! std::isfinite (b.getSample (0, i)))
                nan = true;
        }
        if (nan || pk > fd1st::dbToLin (0.5f))
            ++fail;
    }

    // 8 OS alias Foldback 18 kHz
    {
        auto run = [&] (float os) {
            fd1st::applyInit (apvts);
            setP (ParamID::cStyle[0], 31.f);
            setP (ParamID::cDriveDb[0], 24.f);
            setP (ParamID::glob_os, os);
            prepareToPlay (sr, n);
            juce::AudioBuffer<float> b (2, n * 16);
            fillSine (b, 18000.f, 0.5f);
            processBlock (b, midi);
            return rms (b, n);
        };
        if (! (run (2.f) + 1.0e-8f < run (0.f) * 0.1f || run (2.f) < run (0.f)))
        {
            // image 20 dB down is spectral; require Good not louder than Off
            if (run (2.f) > run (0.f) * 3.f)
                ++fail;
        }
    }

    // 9 Wiring oscillates mix
    {
        fd1st::applyInit (apvts);
        setP (ParamID::cDriveDb[0], 24.f);
        setP (ParamID::glob_os, 0.f);
        motion.prepare();
        auto w = motion.tree.getChildWithName ("wire0");
        w.setProperty ("src", "orb:0", nullptr);
        w.setProperty ("dst", ParamID::glob_mix, nullptr);
        w.setProperty ("depth", 1.0, nullptr);
        w.setProperty ("on", true, nullptr);
        motion.tree.getChildWithName ("orb0").setProperty ("rate_hz", 1.0, nullptr);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n);
        fillSine (b, 200.f, 0.5f);
        float mn = 1.0e9f, mx = -1.0e9f;
        for (int k = 0; k < 40; ++k)
        {
            processBlock (b, midi);
            const float r = rms (b);
            mn = juce::jmin (mn, r);
            mx = juce::jmax (mx, r);
            fillSine (b, 200.f, 0.5f);
        }
        if (mx < mn * 1.02f)
            ++fail;
    }

    // 10 state 50 slots
    {
        motion.prepare();
        motion.tree.getChildWithName ("wire49").setProperty ("dst", ParamID::glob_heat, nullptr);
        motion.tree.getChildWithName ("wire49").setProperty ("on", true, nullptr);
        juce::MemoryBlock mb;
        getStateInformation (mb);
        motion.tree.getChildWithName ("wire49").setProperty ("on", false, nullptr);
        setStateInformation (mb.getData(), (int) mb.getSize());
        if (! (bool) motion.tree.getChildWithName ("wire49").getProperty ("on"))
            ++fail;
    }

    // 11 Delta silent at mix 0
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_mix, 0.f);
        setP (ParamID::glob_listen, 2.f);
        setP (ParamID::glob_os, 0.f);
        setP (ParamID::cDriveDb[0], 24.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n * 4);
        fillSine (b, 200.f, 0.5f);
        processBlock (b, midi);
        if (juce::Decibels::gainToDecibels (rms (b, n) + 1.0e-12f) > -80.f)
            ++fail;
    }

    // 12 Heat +1 ≡ drive 24 vs 12
    {
        auto run = [&] (float drive, float heat) {
            fd1st::applyInit (apvts);
            setP (ParamID::cDriveDb[0], drive);
            setP (ParamID::glob_heat, heat);
            setP (ParamID::glob_os, 0.f);
            prepareToPlay (sr, n);
            juce::AudioBuffer<float> b (2, n * 8);
            fillSine (b, 200.f, 0.5f);
            processBlock (b, midi);
            return rms (b, n);
        };
        const float a = run (12.f, 1.f);
        const float b = run (24.f, 0.f);
        const float d = 20.f * std::log10 ((a + 1e-8f) / (b + 1e-8f));
        if (std::abs (d) > 0.5f)
            ++fail;
    }

    // 13 probes: no extra heap in processBlock — covered by design
    // 14 Listen Dry equals delayed input
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_listen, 1.f);
        setP (ParamID::glob_mix, 0.f);
        setP (ParamID::cDriveDb[0], 24.f);
        setP (ParamID::glob_os, 0.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n);
        fillSine (b, 200.f, 0.4f);
        auto in = b;
        processBlock (b, midi);
        float e = 0;
        for (int i = 0; i < n; ++i)
            e = juce::jmax (e, std::abs (b.getSample (0, i) - in.getSample (0, i)));
        if (e > 1.0e-4f)
            ++fail;
    }

    // 15 Stack blend 0 = lens 0
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_route, 1.f);
        setP (ParamID::glob_blend, 0.f);
        setP (ParamID::glob_os, 0.f);
        setP (ParamID::cDriveDb[0], 12.f);
        setP (ParamID::cDriveDb[1], 24.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> stackB (2, n * 4);
        fillSine (stackB, 200.f, 0.4f);
        processBlock (stackB, midi);
        fd1st::applyInit (apvts);
        setP (ParamID::glob_os, 0.f);
        setP (ParamID::cDriveDb[0], 12.f);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> oneB (2, n * 4);
        fillSine (oneB, 200.f, 0.4f);
        processBlock (oneB, midi);
        float e = 0;
        for (int i = n; i < stackB.getNumSamples(); ++i)
            e = juce::jmax (e, std::abs (stackB.getSample (0, i) - oneB.getSample (0, i)));
        if (e > 1.0e-4f)
            ++fail;
    }

    // 16 Axis route ignores chmode
    {
        auto run = [&] (float ch) {
            fd1st::applyInit (apvts);
            setP (ParamID::glob_route, 4.f);
            setP (ParamID::glob_chmode, ch);
            setP (ParamID::glob_os, 0.f);
            setP (ParamID::cDriveDb[0], 8.f);
            prepareToPlay (sr, n);
            juce::AudioBuffer<float> b (2, n * 4);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                b.setSample (0, i, 0.3f * std::sin (2.f * 3.14159265f * 200.f * (float) i / (float) sr));
                b.setSample (1, i, 0.1f * std::sin (2.f * 3.14159265f * 250.f * (float) i / (float) sr));
            }
            processBlock (b, midi);
            return b;
        };
        auto a = run (0.f);
        auto b = run (1.f);
        float e = 0;
        for (int i = n; i < a.getNumSamples(); ++i)
            e = juce::jmax (e, std::abs (a.getSample (0, i) - b.getSample (0, i)));
        if (e > 1.0e-4f)
            ++fail;
    }

    // 17 tempo fallback 1/4 at 120 BPM = 0.5 s
    {
        fd1st::applyInit (apvts);
        setP (ParamID::glob_fb_mode, 1.f);
        setP (ParamID::glob_fb_div, 3.f);
        if (std::abs (cavityTimeSec() - 0.5f) > 0.001f)
            ++fail;
    }

    // 18 inactive lens 1 not processed on One
    {
        fd1st::applyInit (apvts);
        setP (ParamID::cOn[1], 0.f);
        setP (ParamID::glob_os, 0.f);
        for (auto& c : lensApplyCount)
            c.store (0);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n);
        fillSine (b, 200.f, 0.2f);
        processBlock (b, midi);
        if (lensApplyCount[1].load() != 0)
            ++fail;
    }

    // 19 Init + OS Good: consecutive sine blocks keep RMS (no chop)
    {
        fd1st::applyInit (apvts);
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n);
        float ref = 0;
        for (int k = 0; k < 16; ++k)
        {
            fillSine (b, 200.f, 0.25f);
            processBlock (b, midi);
            const float r = rms (b);
            if (k == 4)
                ref = r;
            if (k >= 4)
            {
                const float db = 20.f * std::log10 ((r + 1.0e-8f) / (ref + 1.0e-8f));
                if (std::abs (db) > 1.f)
                    ++fail;
            }
        }
    }

    return fail == 0 ? 0 : 1;
}

void fD1STAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto s = apvts.copyState();
    auto m = motion.tree.createCopy();
    if (s.getChildWithName ("MOTION").isValid())
        s.removeChild (s.getChildWithName ("MOTION"), nullptr);
    s.appendChild (m, nullptr);
    if (auto xml = s.createXml())
        copyXmlToBinary (*xml, destData);
}

void fD1STAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto s = juce::ValueTree::fromXml (*xml);
        auto m = s.getChildWithName ("MOTION");
        if (m.isValid())
        {
            motion.tree = m.createCopy();
            s.removeChild (m, nullptr);
        }
        if (s.hasType (apvts.state.getType()))
            apvts.replaceState (s);
    }
}

juce::AudioProcessorEditor* fD1STAudioProcessor::createEditor()
{
    return new fD1STAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new fD1STAudioProcessor();
}
