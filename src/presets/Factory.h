#pragma once

#include "../dsp/Names.h"
#include "../parameters/ParameterIDs.h"

#include <JuceHeader.h>

namespace fd1st
{

inline void setF (juce::AudioProcessorValueTreeState& a, const char* id, float v)
{
    if (auto* p = a.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

inline void setChoice (juce::AudioProcessorValueTreeState& a, const char* id, int v)
{
    if (auto* p = a.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) v));
}

inline void applyInit (juce::AudioProcessorValueTreeState& a)
{
    setChoice (a, ParamID::glob_route, 0);
    setF (a, ParamID::glob_drive_db, 0.f);
    setF (a, ParamID::glob_mix, 1.f);
    setF (a, ParamID::glob_heat, 0.f);
    setF (a, ParamID::glob_spread, 0.f);
    setF (a, ParamID::glob_motion, 1.f);
    setChoice (a, ParamID::glob_listen, 0);
    setChoice (a, ParamID::glob_os, 2);
    setF (a, ParamID::cDriveDb[0], 0.f);
    setChoice (a, ParamID::cStyle[0], 0);
    setF (a, ParamID::cMix[0], 1.f);
}

inline const char* factoryName (int i)
{
    static const char* n[] = {
        "Init", "SightWide", "QuietGlue", "MakeupMatch", "OneGlow12",
        "EmberDrive", "PrintTape", "PackBus", "SatLitePad", "GluePress",
        "SplitGrit", "AxisBite", "CavityHum", "KitPrism", "VoicePrism",
        "LowMud", "AirShard", "MonoCore", "SideSpice", "VocalSilk",
        "BassIron", "DrumSmash", "MasterGlue", "ThreeGlass", "SixShard",
        "CavityNote", "RingLens", "HeatWash", "SpreadBus", "MidKind",
        "SideOnly", "CenterPunch", "SafeLoop", "TempoWash", "NoteDrone",
        "OrbitMix", "StrikePop", "FollowHeat", "NoiseSmear", "XyBlend",
        "MidiWah", "DeltaSolo", "CausticAir", "FoldHard", "BitRain",
        "DropOut", "ShardChoir", "RectNight", "FullWreck"
    };
    if (i < 0 || i >= 49)
        return "Init";
    return n[i];
}

inline constexpr int kFactoryCount = 49;

inline void applyFactory (juce::AudioProcessorValueTreeState& a, int index)
{
    applyInit (a);
    if (index <= 0)
        return;
    if (index == 4 || index == 1)
        setF (a, ParamID::cDriveDb[0], 12.f);
    if (index == 3)
        setF (a, ParamID::cDriveDb[0], 24.f);
    if (index == 5)
        setChoice (a, ParamID::cStyle[0], 2);
    if (index == 6)
        setChoice (a, ParamID::cStyle[0], 5);
    if (index == 10)
        setChoice (a, ParamID::glob_route, 2);
    if (index == 11)
        setChoice (a, ParamID::glob_route, 4);
    if (index == 12)
        setChoice (a, ParamID::glob_route, 5);
    if (index == 13)
        setChoice (a, ParamID::glob_route, 3);
    if (index == 31)
        setChoice (a, ParamID::cStyle[0], 31);
    if (index == 33)
        setChoice (a, ParamID::cStyle[0], 33);
}

} // namespace fd1st
