#pragma once

#include "dsp/LensProcessor.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>
#include <array>

class fD1STAudioProcessor : public juce::AudioProcessor
{
public:
    fD1STAudioProcessor();
    ~fD1STAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "fD1ST"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool supportsMPE() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    float loadf (const char* id, float fallback = 0.f) const;
    int osId() const;
    int osLatency() const;
    void processWet (juce::dsp::AudioBlock<float> block);

    fd1st::LensProcessor lens0;
    juce::dsp::Oversampling<float> os2 { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false };
    juce::dsp::Oversampling<float> os8 { 2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false };

    juce::AudioBuffer<float> dryBuf;
    std::array<std::vector<float>, 2> dryDelay {};
    int delayCap = 64;
    int delayLen = 1;
    int delayW = 0;
    int osLatencySamples = 0;
    int lastOs = -1;
    double hostSr = 48000.0;
    int hostBlock = 512;

    juce::SmoothedValue<float> mixSmooth, outSmooth, globDriveSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fD1STAudioProcessor)
};
