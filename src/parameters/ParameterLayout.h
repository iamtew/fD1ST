#pragma once

#include "ParameterIDs.h"
#include <JuceHeader.h>

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamID::out_db, 1 },
        "Out",
        juce::NormalisableRange<float> (-24.f, 24.f, 0.1f),
        0.f));
    return { p.begin(), p.end() };
}
