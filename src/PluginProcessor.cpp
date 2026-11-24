#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <cstdlib>

fD1STAudioProcessor::fD1STAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    maybeSelfTest();
}

void fD1STAudioProcessor::prepareToPlay (double sampleRate, int)
{
    juce::ScopedNoDenormals noDenormals;
    outSmooth.reset (sampleRate, 0.02);
    if (auto* p = apvts.getRawParameterValue (ParamID::out_db))
        outSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p->load()));
}

bool fD1STAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::stereo());
}

void fD1STAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const int n = buffer.getNumSamples();
    const int nCh = juce::jmin (2, buffer.getNumChannels());

    float outDb = 0.f;
    if (auto* p = apvts.getRawParameterValue (ParamID::out_db))
        outDb = p->load();
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outDb));

    for (int i = 0; i < n; ++i)
    {
        const float g = outSmooth.getNextValue();
        for (int ch = 0; ch < nCh; ++ch)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * g);
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
    std::exit (runSelfTest());
}

int fD1STAudioProcessor::runSelfTest()
{
    const double sr = 48000.0;
    const int n = 512;
    prepareToPlay (sr, n);
    juce::MidiBuffer midi;

    {
        juce::AudioBuffer<float> b (2, n);
        b.clear();
        processBlock (b, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < n; ++i)
                if (std::abs (b.getSample (ch, i)) > 1.0e-6f)
                    return 1;
    }

    {
        juce::AudioBuffer<float> b (2, n);
        for (int i = 0; i < n; ++i)
        {
            const float s = 0.25f * std::sin (6.28318530718f * 440.f * (float) i / (float) sr);
            b.setSample (0, i, s);
            b.setSample (1, i, s);
        }
        processBlock (b, midi);
        for (int i = 0; i < n; ++i)
        {
            const float s = 0.25f * std::sin (6.28318530718f * 440.f * (float) i / (float) sr);
            if (std::abs (b.getSample (0, i) - s) > 1.0e-4f)
                return 2;
        }
    }

    {
        if (auto* p = apvts.getParameter (ParamID::out_db))
            p->setValueNotifyingHost (p->convertTo0to1 (0.f));
        prepareToPlay (sr, n);
        juce::AudioBuffer<float> b (2, n);
        for (int i = 0; i < n; ++i)
        {
            b.setSample (0, i, 0.5f);
            b.setSample (1, i, 0.5f);
        }
        processBlock (b, midi);
        for (int i = 0; i < n; ++i)
            if (! std::isfinite (b.getSample (0, i)) || std::abs (b.getSample (0, i)) > 2.f)
                return 3;
    }

    return 0;
}

void fD1STAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void fD1STAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto s = juce::ValueTree::fromXml (*xml);
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
