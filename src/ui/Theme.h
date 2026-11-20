#pragma once

#include <JuceHeader.h>

namespace fd1st
{

// Forest Depth. Main is the plugin: burgundy chrome, not a black skin with a red logo.
inline const juce::Colour kMain     { 0xff900020 };
inline const juce::Colour kEmerald  { 0xff0F6B4E };
inline const juce::Colour kSage     { 0xffA8C5B0 };
inline const juce::Colour kForest   { 0xff1A3C32 };
inline const juce::Colour kBlack    { 0xff000000 };
inline const juce::Colour kWhite    { 0xffffffff };
inline const juce::Colour kWell     { kMain.overlaidWith (kBlack.withAlpha (0.45f)) };

inline juce::Font boldFont (float pt)
{
    return juce::Font (juce::FontOptions (pt).withStyle ("Bold"));
}

} // namespace fd1st
