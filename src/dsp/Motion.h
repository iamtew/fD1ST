#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

namespace fd1st
{

// Orbit 0 + Wiring slot 0. Other sources stay in the tree as empty children for state.
struct Motion
{
    juce::ValueTree tree { "MOTION" };
    float orbPhase = 0;
    float orbRate = 1.f;
    int orbWave = 0;
    bool wireOn = false;
    float wireDepth = 0;
    juce::String wireDst;

    void prepare()
    {
        if (! tree.getChildWithName ("orb0").isValid())
        {
            juce::ValueTree o ("orb0");
            o.setProperty ("wave", 0, nullptr);
            o.setProperty ("rate_hz", 1.0, nullptr);
            tree.appendChild (o, nullptr);
        }
        if (! tree.getChildWithName ("wire0").isValid())
        {
            juce::ValueTree w ("wire0");
            w.setProperty ("src", "orb:0", nullptr);
            w.setProperty ("dst", juce::String(), nullptr);
            w.setProperty ("depth", 0.0, nullptr);
            w.setProperty ("on", false, nullptr);
            tree.appendChild (w, nullptr);
        }
        for (int i = 1; i < 4; ++i)
            if (! tree.getChildWithName ("orb" + juce::String (i)).isValid())
                tree.appendChild (juce::ValueTree ("orb" + juce::String (i)), nullptr);
        for (int i = 0; i < 4; ++i)
            if (! tree.getChildWithName ("stk" + juce::String (i)).isValid())
                tree.appendChild (juce::ValueTree ("stk" + juce::String (i)), nullptr);
        for (int i = 0; i < 4; ++i)
            if (! tree.getChildWithName ("fol" + juce::String (i)).isValid())
                tree.appendChild (juce::ValueTree ("fol" + juce::String (i)), nullptr);
    }

    void pullFromTree()
    {
        if (auto o = tree.getChildWithName ("orb0"); o.isValid())
        {
            orbWave = (int) o.getProperty ("wave");
            orbRate = juce::jlimit (0.01f, 40.f, (float) o.getProperty ("rate_hz"));
        }
        if (auto w = tree.getChildWithName ("wire0"); w.isValid())
        {
            wireOn = (bool) w.getProperty ("on");
            wireDepth = (float) w.getProperty ("depth");
            wireDst = w.getProperty ("dst").toString();
        }
    }

    float orbitSample (float dt)
    {
        orbPhase += orbRate * dt;
        if (orbPhase >= 1.f)
            orbPhase -= std::floor (orbPhase);
        const float p = orbPhase;
        if (orbWave == 1)
            return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
        if (orbWave == 2)
            return 2.f * p - 1.f;
        if (orbWave == 3)
            return 1.f - 2.f * p;
        if (orbWave == 4)
            return p < 0.5f ? 1.f : -1.f;
        return std::sin (p * 6.2831853f);
    }
};

} // namespace fd1st
