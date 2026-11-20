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

inline void setBool (juce::AudioProcessorValueTreeState& a, const char* id, bool v)
{
    if (auto* p = a.getParameter (id))
        p->setValueNotifyingHost (v ? 1.f : 0.f);
}

inline void applyInit (juce::AudioProcessorValueTreeState& a)
{
    setChoice (a, ParamID::glob_route, 0);
    setF (a, ParamID::glob_blend, 0.5f);
    if (auto* p = a.getParameter (ParamID::glob_bands))
        p->setValueNotifyingHost (p->convertTo0to1 (3.f));
    setChoice (a, ParamID::glob_chmode, 0);
    setF (a, ParamID::glob_drive_db, 0.f);
    setF (a, ParamID::glob_tone, 0.f);
    setBool (a, ParamID::glob_colorcomp, true);
    setF (a, ParamID::glob_heat, 0.f);
    setF (a, ParamID::glob_spread, 0.f);
    setF (a, ParamID::glob_motion, 1.f);
    setChoice (a, ParamID::glob_listen, 0);
    setChoice (a, ParamID::glob_fb_mode, 0);
    setF (a, ParamID::glob_fb_time, 0.25f);
    setChoice (a, ParamID::glob_fb_div, 2);
    setF (a, ParamID::glob_fb_note_hz, 110.f);
    setF (a, ParamID::glob_fb_amt, 0.f);
    setBool (a, ParamID::glob_fb_safe, true);
    setF (a, ParamID::glob_comp, 0.f);
    setBool (a, ParamID::glob_comp_hpf, true);
    setF (a, ParamID::glob_out_db, 0.f);
    setF (a, ParamID::glob_mix, 1.f);
    setChoice (a, ParamID::glob_os, 2);
    setBool (a, ParamID::glob_linphase, false);
    setBool (a, ParamID::glob_bypass, false);
    for (int i = 0; i < kXovers; ++i)
    {
        setF (a, ParamID::xoverHz[i], kXoverDefaultHz[i]);
        setChoice (a, ParamID::xoverSlope[i], 2);
    }
    for (int i = 0; i < kLenses; ++i)
    {
        setBool (a, ParamID::cOn[i], i == 0);
        setBool (a, ParamID::cSolo[i], false);
        setBool (a, ParamID::cMute[i], false);
        setChoice (a, ParamID::cStyle[i], 0);
        setF (a, ParamID::cDriveDb[i], i == 0 ? 0.f : 12.f);
        setF (a, ParamID::cDrivePan[i], 0.f);
        setF (a, ParamID::cBias[i], 0.f);
        setF (a, ParamID::cMix[i], 1.f);
        setF (a, ParamID::cDyn[i], 0.f);
        setF (a, ParamID::cBass[i], 0.f);
        setF (a, ParamID::cMid[i], 0.f);
        setF (a, ParamID::cTreble[i], 0.f);
        setF (a, ParamID::cPresence[i], 0.f);
        setChoice (a, ParamID::cFiltType[i], 0);
        setF (a, ParamID::cFiltHz[i], 1200.f);
        setF (a, ParamID::cFiltQ[i], 0.707f);
        setChoice (a, ParamID::cFiltPos[i], 1);
        setF (a, ParamID::cFbAmt[i], 0.f);
        setF (a, ParamID::cFbHz[i], 220.f);
        setF (a, ParamID::cLevelDb[i], 0.f);
        setF (a, ParamID::cPan[i], 0.f);
    }
}

inline const char* factoryName (int i)
{
    static const char* n[] = {
        "Init", "SeeDelta", "QuietMaster", "MakeupMatch",
        "SoftPress", "NightPrint", "PackThe2b", "IronBus", "GluePress",
        "KickWeight", "SnareCrack", "RoomSmash", "HatGlass", "KitPrism",
        "SubKeep", "EightOhGrit", "MonoCore", "GrowlCoil",
        "VocalSilk", "VocalBite", "AirWire", "AmpInTheRoom", "AcousticEmber",
        "ThreeGlass", "SixShard", "LowLaw", "HighLace", "SlopeWall",
        "MidKind", "SideKnives", "CenterPunch",
        "EighthGhost", "DottedTrail", "NoteDrone", "BreathBack", "TripletSmear",
        "FollowHeat", "OrbitPrism", "NoiseBias", "StrikeOpen", "TransientOpen", "XYPerform",
        "FoldTheSun", "BitRain", "DropOut", "ShardChoir", "RectNight", "FullWreck"
    };
    if (i < 0 || i >= 48)
        return "Init";
    return n[i];
}

inline constexpr int kFactoryCount = 48;

inline const char* stampName (int i)
{
    static const char* n[] = {
        "Glow", "Ember", "Print", "Pack", "SatLite", "TfWarm",
        "CrunchAmp", "SoftSine", "Foldback", "Crush", "Smear", "Rectify"
    };
    if (i < 0 || i >= 12)
        return "Glow";
    return n[i];
}

inline constexpr int kStampCount = 12;
inline constexpr int kStampStyle[12] = { 0, 2, 5, 6, 20, 19, 13, 23, 31, 33, 29, 32 };

inline void applyStamp (juce::AudioProcessorValueTreeState& a, int lens, int stamp)
{
    const int i = juce::jlimit (0, 5, lens);
    setChoice (a, ParamID::cStyle[i], kStampStyle[juce::jlimit (0, 11, stamp)]);
    setF (a, ParamID::cDriveDb[i], 12.f);
    setF (a, ParamID::cMix[i], 1.f);
    setBool (a, ParamID::cOn[i], true);
}

inline void onLens (juce::AudioProcessorValueTreeState& a, int i, bool on)
{
    setBool (a, ParamID::cOn[i], on);
}

inline void applyFactory (juce::AudioProcessorValueTreeState& a, int index)
{
    applyInit (a);
    const int i = juce::jlimit (0, 47, index);
    auto style = [&] (int lens, int s, float drive, float mix = 1.f)
    {
        onLens (a, lens, true);
        setChoice (a, ParamID::cStyle[lens], s);
        setF (a, ParamID::cDriveDb[lens], drive);
        setF (a, ParamID::cMix[lens], mix);
    };

    switch (i)
    {
        case 0: break;
        case 1: style (0, 22, 18.f); break;
        case 2:
            style (0, 20, 4.f);
            setF (a, ParamID::glob_mix, 0.35f);
            setBool (a, ParamID::glob_linphase, true);
            setChoice (a, ParamID::glob_os, 3);
            setF (a, ParamID::glob_comp, 0.15f);
            break;
        case 3: style (0, 0, 24.f); break;
        case 4:
            style (0, 20, 6.f);
            setF (a, ParamID::glob_mix, 0.4f);
            setF (a, ParamID::glob_comp, 0.22f);
            break;
        case 5:
            style (0, 5, 8.f);
            setF (a, ParamID::glob_tone, -0.2f);
            setBool (a, ParamID::glob_colorcomp, true);
            break;
        case 6:
            style (0, 6, 10.f);
            setF (a, ParamID::glob_comp, 0.3f);
            setF (a, ParamID::glob_mix, 0.7f);
            break;
        case 7:
            style (0, 19, 7.f);
            setF (a, ParamID::cPresence[0], 2.f);
            setF (a, ParamID::glob_mix, 0.5f);
            break;
        case 8:
            setChoice (a, ParamID::glob_route, 2);
            style (0, 0, 5.f);
            style (1, 22, 12.f, 0.25f);
            setF (a, ParamID::glob_blend, 0.3f);
            setF (a, ParamID::glob_heat, 0.1f);
            break;
        case 9:
            setChoice (a, ParamID::glob_route, 3);
            if (auto* p = a.getParameter (ParamID::glob_bands))
                p->setValueNotifyingHost (p->convertTo0to1 (2.f));
            setF (a, ParamID::xoverHz[0], 180.f);
            style (0, 2, 14.f);
            setF (a, ParamID::cDyn[0], 0.35f);
            style (1, 1, 4.f, 0.4f);
            break;
        case 10:
            style (0, 13, 16.f);
            setChoice (a, ParamID::cFiltType[0], 3);
            setF (a, ParamID::cFiltHz[0], 180.f);
            setChoice (a, ParamID::cFiltPos[0], 0);
            setF (a, ParamID::cPresence[0], 4.f);
            break;
        case 11:
            setChoice (a, ParamID::glob_route, 2);
            style (0, 6, 8.f);
            style (1, 33, 10.f, 0.18f);
            setF (a, ParamID::glob_blend, 0.25f);
            break;
        case 12:
            style (0, 1, 10.f, 0.6f);
            setChoice (a, ParamID::cFiltType[0], 3);
            setF (a, ParamID::cFiltHz[0], 6000.f);
            setChoice (a, ParamID::cFiltPos[0], 0);
            break;
        case 13:
            setChoice (a, ParamID::glob_route, 3);
            setF (a, ParamID::xoverHz[0], 180.f);
            setF (a, ParamID::xoverHz[1], 2500.f);
            style (0, 0, 8.f);
            style (1, 2, 12.f);
            style (2, 1, 6.f);
            setF (a, ParamID::cDyn[1], 0.2f);
            break;
        case 14:
            setChoice (a, ParamID::glob_route, 3);
            if (auto* p = a.getParameter (ParamID::glob_bands))
                p->setValueNotifyingHost (p->convertTo0to1 (2.f));
            setF (a, ParamID::xoverHz[0], 90.f);
            style (0, 20, 3.f, 0.3f);
            style (1, 2, 14.f);
            break;
        case 15:
            style (0, 19, 18.f);
            setChoice (a, ParamID::cFiltType[0], 3);
            setF (a, ParamID::cFiltHz[0], 40.f);
            setF (a, ParamID::glob_comp, 0.25f);
            break;
        case 16:
            setChoice (a, ParamID::glob_route, 4);
            style (0, 0, 8.f);
            setBool (a, ParamID::cMute[1], true);
            onLens (a, 1, true);
            setF (a, ParamID::glob_spread, -0.4f);
            break;
        case 17:
            style (0, 14, 20.f);
            setChoice (a, ParamID::cFiltType[0], 7);
            setF (a, ParamID::cFiltHz[0], 110.f);
            setF (a, ParamID::cFiltQ[0], 3.f);
            setF (a, ParamID::cFbAmt[0], 0.2f);
            setF (a, ParamID::cFbHz[0], 110.f);
            break;
        case 18:
            style (0, 0, 5.f);
            setF (a, ParamID::glob_mix, 0.45f);
            setF (a, ParamID::glob_tone, 0.1f);
            break;
        case 19:
            setChoice (a, ParamID::glob_route, 2);
            style (0, 1, 4.f);
            style (1, 24, 14.f, 0.2f);
            setChoice (a, ParamID::cFiltType[1], 3);
            setF (a, ParamID::cFiltHz[1], 1200.f);
            setChoice (a, ParamID::cFiltPos[1], 0);
            setF (a, ParamID::glob_blend, 0.35f);
            break;
        case 20:
            style (0, 11, 12.f);
            setF (a, ParamID::cPresence[0], 5.f);
            setF (a, ParamID::glob_mix, 0.5f);
            break;
        case 21:
            setChoice (a, ParamID::glob_route, 1);
            style (0, 8, 10.f);
            style (1, 16, 8.f);
            setF (a, ParamID::glob_blend, 0.55f);
            break;
        case 22:
            style (0, 2, 6.f);
            setF (a, ParamID::glob_mix, 0.3f);
            setF (a, ParamID::glob_comp, 0.1f);
            break;
        case 23:
            setChoice (a, ParamID::glob_route, 3);
            setF (a, ParamID::xoverHz[0], 200.f);
            setF (a, ParamID::xoverHz[1], 2000.f);
            style (0, 0, 8.f);
            style (1, 2, 10.f);
            style (2, 1, 6.f);
            break;
        case 24:
            setChoice (a, ParamID::glob_route, 3);
            if (auto* p = a.getParameter (ParamID::glob_bands))
                p->setValueNotifyingHost (p->convertTo0to1 (6.f));
            style (0, 0, 8.f);
            style (1, 0, 8.f);
            style (2, 0, 10.f);
            style (3, 27, 10.f);
            style (4, 0, 8.f);
            style (5, 27, 12.f);
            setF (a, ParamID::glob_heat, 0.2f);
            break;
        case 25:
            setChoice (a, ParamID::glob_route, 3);
            setChoice (a, ParamID::xoverSlope[0], 3);
            setChoice (a, ParamID::xoverSlope[1], 3);
            style (0, 33, 6.f, 0.2f);
            style (1, 5, 8.f);
            style (2, 0, 4.f, 0.15f);
            break;
        case 26:
            setChoice (a, ParamID::glob_route, 3);
            style (0, 20, 4.f);
            style (1, 20, 4.f);
            style (2, 31, 8.f, 0.35f);
            break;
        case 27:
            setChoice (a, ParamID::glob_route, 3);
            if (auto* p = a.getParameter (ParamID::glob_bands))
                p->setValueNotifyingHost (p->convertTo0to1 (4.f));
            setF (a, ParamID::xoverHz[0], 80.f);
            setF (a, ParamID::xoverHz[1], 400.f);
            setF (a, ParamID::xoverHz[2], 4000.f);
            for (int s = 0; s < 3; ++s)
                setChoice (a, ParamID::xoverSlope[s], 3);
            style (0, 0, 8.f);
            style (1, 0, 8.f);
            style (2, 0, 8.f);
            style (3, 0, 8.f);
            setF (a, ParamID::cDyn[0], 0.15f);
            setF (a, ParamID::cDyn[1], 0.15f);
            setF (a, ParamID::cDyn[2], 0.15f);
            setF (a, ParamID::cDyn[3], 0.15f);
            break;
        case 28:
            setChoice (a, ParamID::glob_route, 4);
            style (0, 0, 4.f);
            style (1, 13, 12.f, 0.7f);
            setF (a, ParamID::glob_spread, 0.3f);
            break;
        case 29:
            setChoice (a, ParamID::glob_route, 4);
            style (0, 20, 2.f);
            style (1, 27, 16.f);
            setChoice (a, ParamID::cFiltType[1], 3);
            setF (a, ParamID::cFiltHz[1], 2000.f);
            break;
        case 30:
            setChoice (a, ParamID::glob_route, 4);
            style (0, 22, 14.f);
            setF (a, ParamID::cDyn[0], 0.3f);
            style (1, 0, 2.f);
            break;
        case 31:
            setChoice (a, ParamID::glob_route, 5);
            setChoice (a, ParamID::glob_fb_mode, 1);
            setChoice (a, ParamID::glob_fb_div, 2);
            setF (a, ParamID::glob_fb_amt, 0.32f);
            setF (a, ParamID::glob_comp, 0.4f);
            style (0, 0, 0.f);
            style (1, 2, 12.f);
            break;
        case 32:
            setChoice (a, ParamID::glob_route, 5);
            setChoice (a, ParamID::glob_fb_mode, 3);
            setChoice (a, ParamID::glob_fb_div, 2);
            setF (a, ParamID::glob_fb_amt, 0.28f);
            style (1, 7, 12.f);
            break;
        case 33:
            setChoice (a, ParamID::glob_route, 5);
            setChoice (a, ParamID::glob_fb_mode, 4);
            setF (a, ParamID::glob_fb_note_hz, 110.f);
            setF (a, ParamID::glob_fb_amt, 0.45f);
            setBool (a, ParamID::glob_fb_safe, true);
            style (1, 0, 12.f);
            setF (a, ParamID::cFbAmt[1], 0.3f);
            setF (a, ParamID::cFbHz[1], 110.f);
            break;
        case 34:
            setChoice (a, ParamID::glob_route, 5);
            setChoice (a, ParamID::glob_fb_mode, 0);
            setF (a, ParamID::glob_fb_time, 0.18f);
            setF (a, ParamID::glob_fb_amt, 0.55f);
            setF (a, ParamID::glob_comp, 0.65f);
            break;
        case 35:
            setChoice (a, ParamID::glob_route, 5);
            setChoice (a, ParamID::glob_fb_mode, 2);
            setChoice (a, ParamID::glob_fb_div, 2);
            style (1, 29, 12.f);
            setF (a, ParamID::glob_fb_amt, 0.3f);
            break;
        case 36:
            style (0, 0, 10.f);
            break;
        case 37:
            setChoice (a, ParamID::glob_route, 3);
            break;
        case 38:
            style (0, 2, 12.f);
            setF (a, ParamID::glob_motion, 1.2f);
            break;
        case 39:
            style (0, 6, 8.f);
            break;
        case 40:
            style (0, 0, 8.f);
            break;
        case 41:
            style (0, 0, 8.f);
            break;
        case 42: style (0, 31, 22.f); break;
        case 43:
            style (0, 33, 24.f);
            setChoice (a, ParamID::cFiltType[0], 8);
            setF (a, ParamID::cFiltHz[0], 9000.f);
            setChoice (a, ParamID::cFiltPos[0], 1);
            break;
        case 44:
            style (0, 30, 18.f);
            setF (a, ParamID::glob_mix, 0.8f);
            break;
        case 45:
            setChoice (a, ParamID::glob_route, 2);
            style (0, 23, 8.f);
            style (1, 27, 20.f);
            setF (a, ParamID::glob_blend, 0.4f);
            break;
        case 46:
            style (0, 32, 16.f);
            setF (a, ParamID::cFbAmt[0], 0.35f);
            setF (a, ParamID::cFbHz[0], 55.f);
            break;
        case 47:
            setChoice (a, ParamID::glob_route, 1);
            style (0, 33, 20.f);
            style (1, 15, 14.f);
            setF (a, ParamID::glob_fb_amt, 0.2f);
            setF (a, ParamID::glob_fb_time, 0.09f);
            setF (a, ParamID::glob_heat, 0.35f);
            break;
        default: break;
    }
}

inline juce::File userPresetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("fD1ST").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

inline juce::File factoryPresetFile (int index)
{
    return juce::File::getSpecialLocation (juce::File::currentExecutableFile)
        .getSiblingFile ("assets")
        .getChildFile ("presets")
        .getChildFile (juce::String::formatted ("%02d-%s.fd1st", index, factoryName (index)));
}

inline juce::var paramsToJson (juce::AudioProcessorValueTreeState& a)
{
    auto* obj = new juce::DynamicObject();
    for (auto* p : a.processor.getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            obj->setProperty (rp->getParameterID(), rp->convertFrom0to1 (rp->getValue()));
    }
    return juce::var (obj);
}

inline juce::String presetToJson (juce::AudioProcessorValueTreeState& a, const juce::ValueTree& motion)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("version", 1);
    root->setProperty ("params", paramsToJson (a));
    if (auto xml = motion.createXml())
        root->setProperty ("motion", xml->toString());
    return juce::JSON::toString (juce::var (root), true);
}

inline void jsonToParams (juce::AudioProcessorValueTreeState& a, const juce::var& params)
{
    if (auto* obj = params.getDynamicObject())
        for (auto& p : obj->getProperties())
        {
            const auto id = p.name.toString();
            setF (a, id.toRawUTF8(), (float) p.value);
        }
}

inline bool loadFd1st (juce::AudioProcessorValueTreeState& a, juce::ValueTree& motion, const juce::File& f)
{
    auto v = juce::JSON::parse (f);
    if (! v.isObject())
        return false;
    jsonToParams (a, v.getProperty ("params", {}));
    auto mx = v.getProperty ("motion", {}).toString();
    if (mx.isNotEmpty())
        if (auto xml = juce::XmlDocument::parse (mx))
            motion = juce::ValueTree::fromXml (*xml);
    return true;
}

inline bool saveFd1st (juce::AudioProcessorValueTreeState& a, const juce::ValueTree& motion, const juce::File& f)
{
    return f.replaceWithText (presetToJson (a, motion));
}

} // namespace fd1st
