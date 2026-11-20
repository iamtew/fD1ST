#pragma once

#include "ParameterIDs.h"
#include "../dsp/Names.h"

#include <JuceHeader.h>

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using P = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using I = juce::AudioParameterInt;
    using ID = juce::ParameterID;
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (const char* id, juce::String name, juce::NormalisableRange<float> r, float def)
    {
        layout.add (std::make_unique<P> (ID { id, 1 }, std::move (name), r, def));
    };
    auto addB = [&] (const char* id, juce::String name, bool def)
    {
        layout.add (std::make_unique<B> (ID { id, 1 }, std::move (name), def));
    };
    auto addC = [&] (const char* id, juce::String name, const char* const* names, int count, int def)
    {
        juce::StringArray arr;
        for (int i = 0; i < count; ++i)
            arr.add (names[i]);
        layout.add (std::make_unique<C> (ID { id, 1 }, std::move (name), arr, def));
    };

    addC (ParamID::glob_route, "Route", fd1st::kRouteNames, fd1st::kRoutes, 0);
    addF (ParamID::glob_blend, "Blend", { 0.f, 1.f, 0.001f }, 0.5f);
    layout.add (std::make_unique<I> (ID { ParamID::glob_bands, 1 }, "Bands", 1, 6, 3));
    addC (ParamID::glob_chmode, "Channel", fd1st::kChmodeNames, fd1st::kChmodeCount, 0);
    addF (ParamID::glob_drive_db, "Drive", { -12.f, 36.f, 0.01f }, 0.f);
    addF (ParamID::glob_tone, "Tone", { -1.f, 1.f, 0.001f }, 0.f);
    addB (ParamID::glob_colorcomp, "Color Comp", true);
    addF (ParamID::glob_heat, "Heat", { -1.f, 1.f, 0.001f }, 0.f);
    addF (ParamID::glob_spread, "Spread", { -1.f, 1.f, 0.001f }, 0.f);
    addF (ParamID::glob_motion, "Motion", { 0.f, 2.f, 0.001f }, 1.f);
    addC (ParamID::glob_listen, "Listen", fd1st::kListenNames, fd1st::kListenCount, 0);
    addC (ParamID::glob_fb_mode, "Cavity Mode", fd1st::kFbModeNames, fd1st::kFbModeCount, 0);
    addF (ParamID::glob_fb_time, "Cavity Time",
          juce::NormalisableRange<float> (0.0005f, 2.f, 0.0001f, 0.3f), 0.25f);
    addC (ParamID::glob_fb_div, "Cavity Div", fd1st::kFbDivNames, fd1st::kFbDivCount, 2);
    addF (ParamID::glob_fb_note_hz, "Cavity Note",
          juce::NormalisableRange<float> (27.5f, 4186.f, 0.01f, 0.3f), 110.f);
    addF (ParamID::glob_fb_amt, "Cavity Amt", { 0.f, 1.f, 0.001f }, 0.f);
    addB (ParamID::glob_fb_safe, "Cavity Safe", true);
    addF (ParamID::glob_comp, "Comp", { 0.f, 1.f, 0.001f }, 0.f);
    addB (ParamID::glob_comp_hpf, "Comp HPF", true);
    addF (ParamID::glob_out_db, "Output", { -24.f, 24.f, 0.01f }, 0.f);
    addF (ParamID::glob_mix, "Mix", { 0.f, 1.f, 0.001f }, 1.f);
    addC (ParamID::glob_os, "OS", fd1st::kOsNames, fd1st::kOsCount, 2);
    addB (ParamID::glob_linphase, "Lin Phase", false);
    addB (ParamID::glob_bypass, "Bypass", false);

    for (int i = 0; i < fd1st::kXovers; ++i)
    {
        const auto n = juce::String (i + 1);
        addF (ParamID::xoverHz[i], "Xover " + n + " Hz",
              juce::NormalisableRange<float> (20.f, 20000.f, 0.1f, 0.4f),
              fd1st::kXoverDefaultHz[i]);
        addC (ParamID::xoverSlope[i], "Xover " + n + " Slope",
              fd1st::kSlopeNames, fd1st::kSlopeCount, 2);
    }

    for (int i = 0; i < fd1st::kLenses; ++i)
    {
        const auto n = juce::String (i);
        addB (ParamID::cOn[i], "L" + n + " On", i == 0);
        addB (ParamID::cSolo[i], "L" + n + " Solo", false);
        addB (ParamID::cMute[i], "L" + n + " Mute", false);
        addC (ParamID::cStyle[i], "L" + n + " Style",
              fd1st::kStyleNames, fd1st::kStyles, 0);
        // Layout default 12 dB for new lenses; Init program uses 0 on lens 0.
        addF (ParamID::cDriveDb[i], "L" + n + " Drive", { 0.f, 36.f, 0.01f }, i == 0 ? 0.f : 12.f);
        addF (ParamID::cDrivePan[i], "L" + n + " Drive Pan", { -1.f, 1.f, 0.001f }, 0.f);
        addF (ParamID::cBias[i], "L" + n + " Bias", { -1.f, 1.f, 0.001f }, 0.f);
        addF (ParamID::cMix[i], "L" + n + " Mix", { 0.f, 1.f, 0.001f }, 1.f);
        addF (ParamID::cDyn[i], "L" + n + " Dyn", { -1.f, 1.f, 0.001f }, 0.f);
        addF (ParamID::cBass[i], "L" + n + " Bass", { -12.f, 12.f, 0.01f }, 0.f);
        addF (ParamID::cMid[i], "L" + n + " Mid", { -12.f, 12.f, 0.01f }, 0.f);
        addF (ParamID::cTreble[i], "L" + n + " Treble", { -12.f, 12.f, 0.01f }, 0.f);
        addF (ParamID::cPresence[i], "L" + n + " Presence", { -12.f, 12.f, 0.01f }, 0.f);
        addC (ParamID::cFiltType[i], "L" + n + " Filt",
              fd1st::kFiltTypeNames, fd1st::kFiltTypeCount, 0);
        addF (ParamID::cFiltHz[i], "L" + n + " Filt Hz",
              juce::NormalisableRange<float> (20.f, 20000.f, 0.1f, 0.4f), 1200.f);
        addF (ParamID::cFiltQ[i], "L" + n + " Filt Q", { 0.3f, 8.f, 0.001f }, 0.707f);
        addC (ParamID::cFiltPos[i], "L" + n + " Filt Pos",
              fd1st::kFiltPosNames, fd1st::kFiltPosCount, 1);
        addF (ParamID::cFbAmt[i], "L" + n + " Ring", { 0.f, 1.f, 0.001f }, 0.f);
        addF (ParamID::cFbHz[i], "L" + n + " Ring Hz",
              juce::NormalisableRange<float> (20.f, 4000.f, 0.1f, 0.4f), 220.f);
        addF (ParamID::cLevelDb[i], "L" + n + " Level", { -60.f, 36.f, 0.01f }, 0.f);
        addF (ParamID::cPan[i], "L" + n + " Pan", { -1.f, 1.f, 0.001f }, 0.f);
    }

    return layout;
}
