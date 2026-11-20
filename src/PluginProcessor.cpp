#include "PluginProcessor.h"
#include "PluginEditor.h"

fD1STAudioProcessor::fD1STAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
}

void fD1STAudioProcessor::prepareToPlay (double sampleRate, int)
{
    mixSmooth.reset (sampleRate, 0.015);
    outSmooth.reset (sampleRate, 0.015);
    mixSmooth.setCurrentAndTargetValue (1.f);
    outSmooth.setCurrentAndTargetValue (1.f);
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

void fD1STAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Sidechain is declared; step 1 does not read it.
    auto main = getBusBuffer (buffer, true, 0);
    const int n = main.getNumSamples();
    const int chans = main.getNumChannels();

    if (loadf (ParamID::glob_bypass) > 0.5f)
        return;

    mixSmooth.setTargetValue (juce::jlimit (0.f, 1.f, loadf (ParamID::glob_mix, 1.f)));
    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (loadf (ParamID::glob_out_db)));

    // Stereo thru. Mix is dry/wet; wet equals dry until Glow lands, so mix is silent.
    // Output gain is live so the box is not dead.
    for (int i = 0; i < n; ++i)
    {
        const float mix = mixSmooth.getNextValue();
        const float outG = outSmooth.getNextValue();
        for (int ch = 0; ch < chans; ++ch)
        {
            float* p = main.getWritePointer (ch);
            const float dry = p[i];
            const float wet = dry;
            p[i] = (dry + (wet - dry) * mix) * outG;
        }
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
