#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

class PathView : public juce::Component, private juce::Timer
{
public:
    explicit PathView (fD1STAudioProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        using namespace fd1st;
        auto r = getLocalBounds().toFloat();
        g.setColour (kForest);
        g.fillRoundedRectangle (r, 6.f);
        const int route = (int) proc.apvts.getRawParameterValue (ParamID::glob_route)->load();
        const float inR = proc.nodeRms[0].load();
        const float outR = proc.nodeRms[1].load();
        const juce::String names[] = { "One", "Stack", "Split", "Prism", "Axis", "Cavity" };
        g.setColour (kSage);
        g.setFont (12.f);
        g.drawText (route >= 0 && route < 6 ? names[route] : "One", r.removeFromTop (16), juce::Justification::centred);

        auto node = [&] (juce::Rectangle<float> b, const juce::String& lab, float rms)
        {
            g.setColour (kEmerald.withAlpha (0.25f + 0.75f * juce::jlimit (0.f, 1.f, rms)));
            g.fillRoundedRectangle (b, 4.f);
            g.setColour (kSage);
            g.drawText (lab, b.toNearestInt(), juce::Justification::centred);
        };

        auto row = r.reduced (8.f);
        const float w = 64.f;
        auto n0 = row.removeFromLeft (w).withSizeKeepingCentre (w, 28);
        row.removeFromLeft (12);
        auto n1 = row.removeFromLeft (w).withSizeKeepingCentre (w, 28);
        row.removeFromLeft (12);
        auto n2 = row.removeFromLeft (w).withSizeKeepingCentre (w, 28);
        g.setColour (kSage.withAlpha (0.5f));
        g.drawLine (n0.getRight(), n0.getCentreY(), n1.getX(), n1.getCentreY(), 1.5f + 3.f * inR);
        g.setColour (kEmerald.withAlpha (0.8f));
        g.drawLine (n1.getRight(), n1.getCentreY(), n2.getX(), n2.getCentreY(), 1.5f + 3.f * outR);
        node (n0, "In", inR);
        node (n1, route == 4 ? "Mid/Side" : "Lens", juce::jmax (inR, outR));
        node (n2, "Out", outR);
    }

    void timerCallback() override { repaint(); }

private:
    fD1STAudioProcessor& proc;
};
