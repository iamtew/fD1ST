#pragma once

#include "dsp/LensProcessor.h"
#include "dsp/Motion.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <vector>

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

    int getNumPrograms() override;
    int getCurrentProgram() override { return program; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    fd1st::Motion motion;
    std::atomic<float> inPeak { 0 }, outPeak { 0 };
    std::atomic<int> focusLens { 0 };
    std::array<std::atomic<float>, 8> nodeRms {};
    int runSelfTest();

private:
    float loadf (const char* id, float fallback = 0.f) const;
    int osId() const;
    int osLatency() const;
    void processWet (juce::dsp::AudioBlock<float> block);
    void processFrame (float& L, float& R);
    void configureLenses();
    void applyMotion (float dt);
    std::atomic<float> mixMod { 0 };
    void maybeSelfTest();

    std::array<fd1st::LensProcessor, 6> lenses;
    juce::dsp::Oversampling<float> os2 { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false };
    juce::dsp::Oversampling<float> os8 { 2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false, false };

    juce::AudioBuffer<float> dryBuf;
    std::array<std::vector<float>, 2> dryDelay {};
    std::array<std::vector<float>, 2> cavity {};
    int delayCap = 64, delayLen = 1, delayW = 0;
    int cavCap = 64, cavW = 0;
    int osLatencySamples = 0;
    int lastOs = -1;
    int program = 0;
    double hostSr = 48000.0;
    int hostBlock = 512;
    float prismLp[5][2] {};
    float cavZ[2] {};
    float causticZ = 0;
    float spreadZ[2] {};
    bool selfTestRan = false;

    juce::SmoothedValue<float> mixSmooth, outSmooth, globDriveSmooth, blendSmooth, heatSmooth, spreadSmooth, fbAmtSmooth, compSmooth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fD1STAudioProcessor)
};
