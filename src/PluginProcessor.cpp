#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/Clip.h"

#include <algorithm>

fD1STAudioProcessor::fD1STAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

int fD1STAudioProcessor::osId() const
{
    int id = (int) loadf (ParamID::glob_os, 2.f);
    if (id < 0)
        id = 0;
    if (id >= 3)
        id = 2; // Superb aliases Good until step 5
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
    lens0.prepare (osSr, hostBlock * factor);

    mixSmooth.reset (hostSr, 0.015);
    outSmooth.reset (osSr, 0.015);
    globDriveSmooth.reset (osSr, 0.015);
    mixSmooth.setCurrentAndTargetValue (1.f);
    outSmooth.setCurrentAndTargetValue (1.f);
    globDriveSmooth.setCurrentAndTargetValue (1.f);

    dryBuf.setSize (2, juce::jmax (4096, hostBlock), false, false, true);
    lastOs = osId();
    osLatencySamples = osLatency();
    setLatencySamples (osLatencySamples);

    delayCap = juce::jmax (64, (int) std::ceil (os8.getLatencyInSamples()) + 8);
    delayLen = juce::jmax (1, osLatencySamples);
    for (auto& d : dryDelay)
        d.assign ((size_t) delayCap, 0.f);
    delayW = 0;
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

void fD1STAudioProcessor::processWet (juce::dsp::AudioBlock<float> block)
{
    const int n = (int) block.getNumSamples();
    const int chans = (int) block.getNumChannels();
    const bool on = loadf (ParamID::cOn[0]) > 0.5f;
    const bool mute = loadf (ParamID::cMute[0]) > 0.5f;
    const float driveDb = loadf (ParamID::cDriveDb[0]);
    const float bias = loadf (ParamID::cBias[0]);
    const float lensMix = loadf (ParamID::cMix[0], 1.f);
    globDriveSmooth.setTargetValue (fd1st::dbToLin (loadf (ParamID::glob_drive_db)));
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (loadf (ParamID::glob_out_db)));

    for (int i = 0; i < n; ++i)
    {
        const float gDrive = globDriveSmooth.getNextValue();
        const float outG = outSmooth.getNextValue();
        lens0.tick (on, mute, driveDb, bias, lensMix);
        for (int ch = 0; ch < chans; ++ch)
        {
            float x = block.getSample (ch, i) * gDrive;
            x = lens0.apply (x);
            x = fd1st::railClip (x * outG);
            if (! std::isfinite (x))
                x = 0.f;
            block.setSample (ch, i, x);
        }
    }
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
        lens0.setRate (osSr);
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
    for (int ch = 0; ch < chans; ++ch)
        dryBuf.copyFrom (ch, 0, main, ch, 0, copyN);

    juce::dsp::AudioBlock<float> block (main);

    if (id == 0)
    {
        processWet (block);
    }
    else
    {
        auto& os = (id == 1) ? os2 : os8;
        auto up = os.processSamplesUp (block);
        processWet (up);
        os.processSamplesDown (block);
    }

    mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, loadf (ParamID::glob_mix, 1.f)));
    const int len = delayLen;
    for (int i = 0; i < n; ++i)
    {
        const float mix = mixSmooth.getNextValue();
        for (int ch = 0; ch < chans; ++ch)
        {
            const float wet = main.getSample (ch, i);
            const float dryIn = dryBuf.getSample (ch, i);
            float dry = dryIn;
            if (len > 1)
            {
                dry = dryDelay[(size_t) ch][(size_t) delayW];
                dryDelay[(size_t) ch][(size_t) delayW] = dryIn;
            }
            main.setSample (ch, i, dry + (wet - dry) * mix);
        }
        if (len > 1)
            delayW = (delayW + 1) % len;
    }
}

void fD1STAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void fD1STAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* fD1STAudioProcessor::createEditor()
{
    return new fD1STAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new fD1STAudioProcessor();
}
