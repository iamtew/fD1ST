#pragma once

#include "dsp/Comp.h"
#include "dsp/FirXover.h"
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
    std::atomic<float> inPeak { 0 }, outPeak { 0 }, lastGr { 1 };
    std::atomic<int> focusLens { 0 };
    std::array<std::atomic<float>, 8> nodeRms {};
    std::array<std::atomic<int>, 6> lensApplyCount {};
    static constexpr int kProbeN = 2048;
    std::array<float, kProbeN> probeIn {}, probeOut {};
    std::atomic<int> probeW { 0 };
    std::atomic<int> cavIndex { 0 };
    std::atomic<float> scPeak { 0 };
    bool detectListen = false;
    std::atomic<bool> midiLearnOn { false };
    juce::String midiLearnId;
    int runSelfTest();

    float loadf (const char* id, float fallback = 0.f) const;
    int osId() const;
    int osFactor() const;
    bool linphaseOn() const;

private:
    int osLatency() const;
    juce::dsp::Oversampling<float>* currentOs();
    void processWet (juce::dsp::AudioBlock<float> block);
    void processFrame (float& L, float& R);
    void configureLenses();
    void applyMotion (float dt, float mainL, float mainR, float scL, float scR);
    void maybeSelfTest();
    void updateOsRate();
    void updatePrism();
    void ensureHostSize (int n);
    void cacheWet();
    float cavityTimeSec() const;
    double hostBpm() const;
    void prismSplit (int split, float inL, float inR, float osSr, float& loL, float& loR, float& hiL, float& hiR);

    std::atomic<float> mixMod { 0 };
    std::array<fd1st::LensProcessor, 6> lenses;
    using Os = juce::dsp::Oversampling<float>;
    // JUCE halfband OS factor is log2: 1=2×, 3=8×, 4=16× (5=32× asserts).
    Os os2i { 2, 1, Os::filterHalfBandPolyphaseIIR, false, false };
    Os os8i { 2, 3, Os::filterHalfBandPolyphaseIIR, false, false };
    Os os32i { 2, 4, Os::filterHalfBandPolyphaseIIR, false, false };
    Os os2f { 2, 1, Os::filterHalfBandFIREquiripple, false, false };
    Os os8f { 2, 3, Os::filterHalfBandFIREquiripple, false, false };
    Os os32f { 2, 4, Os::filterHalfBandFIREquiripple, false, false };

    juce::AudioBuffer<float> dryBuf;
    std::array<std::vector<float>, 2> dryDelay {};
    std::array<std::vector<float>, 2> cavity {};
    int delayCap = 64, delayLen = 1, delayW = 0;
    int cavCap = 64, cavW = 0;
    int osLatencySamples = 0;
    int lastOs = -1;
    int lastLin = -1;
    int program = 0;
    double hostSr = 48000.0;
    int hostBlock = 512;
    double cachedBpm = 120.0;
    struct WetCache
    {
        int route = 0, chmode = 0, bands = 3;
        bool color = true, safe = true, hpf = true, anySolo = false;
        bool solo[6] {};
        float globComp = 0, cavDel = 1.f;
        struct Lens
        {
            float pan = 0, driveDb = 0, drivePan = 0, bias = 0, mix = 1.f;
            float levelDb = 0, fbAmt = 0, fbHz = 220.f, dyn = 0;
        } lens[6];
    } wet;
    float prismLp[5][2] {}, prismLp2[5][2] {};
    float toneZ[2] {}, tonePostZ[2] {};
    float cavHp[2] {};
    float causticZ = 0;
    bool selfTestRan = false;

    juce::dsp::LinkwitzRileyFilter<float> lrLo[5], lrHi[5], lrLo2[5], lrHi2[5];
    fd1st::FirSplit fir[5];
    fd1st::Comp cavCompL, cavCompR, outCompL, outCompR;
    float lastXoverHz[5] {};
    int lastXoverSlope[5] {};

    juce::SmoothedValue<float> mixSmooth, outSmooth, globDriveSmooth, blendSmooth, heatSmooth, spreadSmooth, fbAmtSmooth, compSmooth, toneSmooth;

    float scL = 0, scR = 0;
    juce::MidiBuffer midiNow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fD1STAudioProcessor)
};
