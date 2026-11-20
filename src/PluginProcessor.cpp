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
    int id = (int) loadf (ParamID::glob_os, 2.f);
    if (id < 0)
        id = 0;
    if (id >= 3)
        id = 2;
    return id;
}

int fD1STAudioProcessor::osLatency() const
{
    const int id = osId();
    if (id == 1)
        return (int) std::ceil (os2.getLatencyInSamples());
    if (id >= 2)
        return (int) std::ceil (os8.getLatencyInSamples());
    return 0;
}

void fD1STAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hostSr = sampleRate > 1.0 ? sampleRate : 48000.0;
    hostBlock = juce::jmax (32, samplesPerBlock);

    os2.initProcessing ((size_t) hostBlock);
    os8.initProcessing ((size_t) hostBlock);
    os2.reset();
    os8.reset();

    const int factor = osId() == 0 ? 1 : (osId() == 1 ? 2 : 8);
    const double osSr = hostSr * (double) factor;
    for (auto& L : lenses)
        L.prepare (osSr, hostBlock * factor);

    mixSmooth.reset (hostSr, 0.015);
    outSmooth.reset (osSr, 0.015);
    globDriveSmooth.reset (osSr, 0.015);
    blendSmooth.reset (osSr, 0.015);
    heatSmooth.reset (osSr, 0.015);
    spreadSmooth.reset (osSr, 0.015);
    fbAmtSmooth.reset (osSr, 0.015);
    compSmooth.reset (osSr, 0.015);
    mixSmooth.setCurrentAndTargetValue (1.f);
    outSmooth.setCurrentAndTargetValue (1.f);
    globDriveSmooth.setCurrentAndTargetValue (1.f);
    blendSmooth.setCurrentAndTargetValue (0.5f);

    dryBuf.setSize (2, juce::jmax (4096, hostBlock), false, false, true);
    lastOs = osId();
    osLatencySamples = osLatency();
    setLatencySamples (osLatencySamples);

    delayCap = juce::jmax (64, (int) std::ceil (os8.getLatencyInSamples()) + 8);
    delayLen = juce::jmax (1, osLatencySamples);
    for (auto& d : dryDelay)
        d.assign ((size_t) delayCap, 0.f);
    delayW = 0;

    cavCap = juce::jmax (64, (int) (2.0 * osSr) + 8);
    for (auto& c : cavity)
        c.assign ((size_t) cavCap, 0.f);
    cavW = 0;
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

void fD1STAudioProcessor::configureLenses()
{
    const float heat = loadf (ParamID::glob_heat);
    const bool safe = loadf (ParamID::glob_fb_safe) > 0.5f;
    for (int i = 0; i < fd1st::kLenses; ++i)
    {
        lenses[(size_t) i].configure (
            loadf (ParamID::cOn[i]) > 0.5f,
            loadf (ParamID::cMute[i]) > 0.5f,
            (int) loadf (ParamID::cStyle[i]),
            loadf (ParamID::cDriveDb[i]),
            heat,
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
    const int route = (int) loadf (ParamID::glob_route);
    const float blend = blendSmooth.getNextValue();
    const float spread = spreadSmooth.getNextValue();
    const float gDrive = globDriveSmooth.getNextValue();
    L *= gDrive;
    R *= gDrive;

    bool ticked[6] = {};
    auto go = [&] (int i, float x, int ch) -> float
    {
        if (loadf (ParamID::cOn[i]) < 0.5f && route == 0 && i != 0)
            return x;
        if (! ticked[i])
        {
            lenses[(size_t) i].tick (loadf (ParamID::cDriveDb[i]),
                                     loadf (ParamID::cBias[i]),
                                     loadf (ParamID::cMix[i], 1.f),
                                     loadf (ParamID::cLevelDb[i]),
                                     loadf (ParamID::cPan[i]),
                                     loadf (ParamID::cFbAmt[i]),
                                     loadf (ParamID::cFbHz[i], 220.f),
                                     loadf (ParamID::cDyn[i]));
            ticked[i] = true;
        }
        return lenses[(size_t) i].apply (x, ch);
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
        const int bands = juce::jlimit (1, 6, (int) loadf (ParamID::glob_bands, 3.f));
        const float osSr = (float) hostSr * (float) (osId() == 0 ? 1 : (osId() == 1 ? 2 : 8));
        oL = oR = 0;
        float restL = L, restR = R;
        for (int b = 0; b < bands; ++b)
        {
            float bandL = restL, bandR = restR;
            if (b + 1 < bands)
            {
                const float hz = loadf (ParamID::xoverHz[b], fd1st::kXoverDefaultHz[b]);
                const float a = fd1st::coeffAt (hz, osSr);
                const float lpL = fd1st::lp1 (restL, prismLp[b][0], a);
                const float lpR = fd1st::lp1 (restR, prismLp[b][1], a);
                bandL = lpL;
                bandR = lpR;
                restL = restL - lpL;
                restR = restR - lpR;
            }
            oL += go (b, bandL, 0);
            oR += go (b, bandR, 1);
        }
        const float loG = fd1st::dbToLin (spread * 3.f);
        const float hiG = fd1st::dbToLin (-spread * 3.f);
        oL *= (loG + hiG) * 0.5f;
        oR *= (loG + hiG) * 0.5f;
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
        const float amt = fbAmtSmooth.getNextValue();
        const float t = loadf (ParamID::glob_fb_time, 0.25f);
        const int osF = osId() == 0 ? 1 : (osId() == 1 ? 2 : 8);
        const int del = juce::jlimit (1, cavCap - 2, (int) (t * hostSr * (double) osF));
        const int r = (cavW - del + cavCap) % cavCap;
        const float fbL = cavity[0][(size_t) r];
        const float fbR = cavity[1][(size_t) r];
        float inL = L + amt * fbL;
        float inR = R + amt * fbR;
        inL = go (1, inL, 0);
        inR = go (1, inR, 1);
        cavity[0][(size_t) cavW] = inL;
        cavity[1][(size_t) cavW] = inR;
        cavW = (cavW + 1) % cavCap;
        oL = fwdL + (inL - fwdL) * blend;
        oR = fwdR + (inR - fwdR) * blend;
    }

    if (route != 5)
    {
        const float amt = fbAmtSmooth.getNextValue();
        if (amt > 0.0001f)
        {
            const float t = loadf (ParamID::glob_fb_time, 0.25f);
            const int osF = osId() == 0 ? 1 : (osId() == 1 ? 2 : 8);
            const int del = juce::jlimit (1, cavCap - 2, (int) (t * hostSr * (double) osF));
            const int r = (cavW - del + cavCap) % cavCap;
            const float fbL = cavity[0][(size_t) r];
            const float fbR = cavity[1][(size_t) r];
            cavity[0][(size_t) cavW] = oL;
            cavity[1][(size_t) cavW] = oR;
            cavW = (cavW + 1) % cavCap;
            oL += amt * fbL;
            oR += amt * fbR;
        }
    }

    const float outG = outSmooth.getNextValue();
    L = fd1st::railClip (oL * outG);
    R = fd1st::railClip (oR * outG);
    if (! std::isfinite (L))
        L = 0.f;
    if (! std::isfinite (R))
        R = 0.f;
}

void fD1STAudioProcessor::processWet (juce::dsp::AudioBlock<float> block)
{
    const int n = (int) block.getNumSamples();
    const int chans = (int) block.getNumChannels();
    configureLenses();
    motion.pullFromTree();
    globDriveSmooth.setTargetValue (fd1st::dbToLin (loadf (ParamID::glob_drive_db)));
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (loadf (ParamID::glob_out_db)));
    blendSmooth.setTargetValue (loadf (ParamID::glob_blend, 0.5f));
    heatSmooth.setTargetValue (loadf (ParamID::glob_heat));
    spreadSmooth.setTargetValue (loadf (ParamID::glob_spread));
    fbAmtSmooth.setTargetValue (loadf (ParamID::glob_fb_amt));
    const float dt = 1.f / (float) (hostSr * (osId() == 0 ? 1 : (osId() == 1 ? 2 : 8)));

    for (int i = 0; i < n; ++i)
    {
        applyMotion (dt);
        float L = block.getSample (0, i);
        float R = chans > 1 ? block.getSample (1, i) : L;
        processFrame (L, R);
        block.setSample (0, i, L);
        if (chans > 1)
            block.setSample (1, i, R);
    }
}

void fD1STAudioProcessor::applyMotion (float dt)
{
    const float src = motion.orbitSample (dt);
    const float depth = motion.wireDepth * loadf (ParamID::glob_motion, 1.f);
    if (motion.wireOn && motion.wireDst == ParamID::glob_mix)
        mixMod.store (src * depth * 0.5f, std::memory_order_relaxed);
    else
        mixMod.store (0.f, std::memory_order_relaxed);
}

void fD1STAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto main = getBusBuffer (buffer, true, 0);
    const int n = main.getNumSamples();
    const int chans = juce::jmin (2, main.getNumChannels());

    const int id = osId();
    if (id != lastOs)
    {
        lastOs = id;
        const int factor = id == 0 ? 1 : (id == 1 ? 2 : 8);
        const double osSr = hostSr * (double) factor;
        for (auto& L : lenses)
            L.setRate (osSr);
        outSmooth.reset (osSr, 0.015);
        globDriveSmooth.reset (osSr, 0.015);
        os2.reset();
        os8.reset();
        osLatencySamples = osLatency();
        setLatencySamples (osLatencySamples);
        delayLen = juce::jmax (1, juce::jmin (delayCap, osLatencySamples));
        for (auto& d : dryDelay)
            std::fill (d.begin(), d.end(), 0.f);
        delayW = 0;
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

    juce::dsp::AudioBlock<float> block (main);
    if (id == 0)
        processWet (block);
    else
    {
        auto& os = (id == 1) ? os2 : os8;
        auto up = os.processSamplesUp (block);
        processWet (up);
        os.processSamplesDown (block);
    }

    const int listen = (int) loadf (ParamID::glob_listen);
    mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, loadf (ParamID::glob_mix, 1.f)));
    const int len = delayLen;
    const float osSr = (float) hostSr;
    float outP = outPeak.load (std::memory_order_relaxed) * 0.9f;
    for (int i = 0; i < copyN; ++i)
    {
        float mix = juce::jlimit (0.f, 1.f, mixSmooth.getNextValue() + mixMod.load (std::memory_order_relaxed));
        for (int ch = 0; ch < chans; ++ch)
        {
            float wet = main.getSample (ch, i);
            float dryIn = dryBuf.getSample (ch, i);
            float dry = dryIn;
            if (len > 1)
            {
                dry = dryDelay[(size_t) ch][(size_t) delayW];
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
                y = fd1st::hp1 (dlt, causticZ, fd1st::coeffAt (1200.f, osSr)) * fd1st::dbToLin (6.f);
            }
            if (listen != 1)
                y = dry + (y - dry) * mix;
            if (listen == 2 && mix <= 0.0001f)
                y = 0.f;
            main.setSample (ch, i, y);
            outP = juce::jmax (outP, std::abs (y));
        }
        if (len > 1)
            delayW = (delayW + 1) % len;
    }
    outPeak.store (outP, std::memory_order_relaxed);
    nodeRms[0].store (inP, std::memory_order_relaxed);
    nodeRms[1].store (outP, std::memory_order_relaxed);
}

int fD1STAudioProcessor::runSelfTest()
{
    const int n = 512;
    const double sr = 48000.0;
    prepareToPlay (sr, n);
    juce::AudioBuffer<float> buf (2, n);
    juce::MidiBuffer midi;
    int fail = 0;

    auto rms = [] (juce::AudioBuffer<float>& b)
    {
        double a = 0;
        for (int c = 0; c < b.getNumChannels(); ++c)
            for (int i = 0; i < b.getNumSamples(); ++i)
                a += (double) b.getSample (c, i) * b.getSample (c, i);
        return (float) std::sqrt (a / (b.getNumChannels() * b.getNumSamples()));
    };

    fd1st::applyInit (apvts);
    buf.clear();
    processBlock (buf, midi);
    if (juce::Decibels::gainToDecibels (rms (buf) + 1.0e-12f) > -90.f)
        ++fail;

    buf.clear();
    for (int i = 0; i < n; ++i)
    {
        const float s = 0.25f * std::sin (2.f * 3.14159265f * 200.f * (float) i / (float) sr);
        buf.setSample (0, i, s);
        buf.setSample (1, i, s);
    }
    if (auto* p = apvts.getParameter (ParamID::glob_bypass))
        p->setValueNotifyingHost (1.f);
    auto copy = buf;
    processBlock (buf, midi);
    float err = 0;
    for (int i = 0; i < n; ++i)
        err = juce::jmax (err, std::abs (buf.getSample (0, i) - copy.getSample (0, i)));
    if (err > 1.0e-5f)
        ++fail;
    if (auto* p = apvts.getParameter (ParamID::glob_bypass))
        p->setValueNotifyingHost (0.f);

    auto sineRms = [&] (float drive) -> float
    {
        fd1st::applyInit (apvts);
        if (auto* p = apvts.getParameter (ParamID::cDriveDb[0]))
            p->setValueNotifyingHost (p->convertTo0to1 (drive));
        if (auto* p = apvts.getParameter (ParamID::glob_os))
            p->setValueNotifyingHost (p->convertTo0to1 (0.f));
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n * 8);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float s = 0.251f * std::sin (2.f * 3.14159265f * 200.f * (float) i / (float) sr);
            b.setSample (0, i, s);
            b.setSample (1, i, s);
        }
        processBlock (b, midi);
        return rms (b);
    };
    const float r0 = sineRms (0.f);
    const float r24 = sineRms (24.f);
    const float db = 20.f * std::log10 ((r24 + 1.0e-8f) / (r0 + 1.0e-8f));
    if (std::abs (db) > 6.f) // 3 dB at 0 dBFS; -12 dBFS probe is looser
        ++fail;

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
