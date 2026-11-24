#pragma once

#include "PluginProcessor.h"
#include "ui/Theme.h"

class fD1STAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit fD1STAudioProcessorEditor (fD1STAudioProcessor&);
    ~fD1STAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    fD1STAudioProcessor& proc;
    juce::Image logo;
    juce::Slider out;
    juce::Label title, spoken, outLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fD1STAudioProcessorEditor)
};
