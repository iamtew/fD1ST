#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

// Living Path. Click a node to Focus that stage. Codes: lens 0–5, else In/Drive/Cavity/Comp/Clip/Out.
class PathView : public juce::Component, private juce::Timer
{
public:
    enum Stage : int
    {
        In = -1,
        Drive = -2,
        Cavity = -3,
        Comp = -4,
        Clip = -5,
        Out = -6
    };

    explicit PathView (fD1STAudioProcessor& p) : proc (p) { startTimerHz (60); }

    int selected = Drive;
    int gutterLeft = 96;

    void paint (juce::Graphics& g) override
    {
        using namespace fd1st;
        hits.clear();
        auto r = getLocalBounds().toFloat();
        g.setColour (kForest);
        g.fillRoundedRectangle (r, 6.f);
        r.removeFromLeft ((float) gutterLeft);

        const int route = (int) proc.apvts.getRawParameterValue (ParamID::glob_route)->load();
        const int bands = juce::jlimit (1, 6, (int) proc.apvts.getRawParameterValue (ParamID::glob_bands)->load());
        const float inR = proc.nodeRms[0].load();
        const float outR = proc.nodeRms[1].load();
        const float amt = proc.apvts.getRawParameterValue (ParamID::glob_fb_amt)->load();
        const float gr = proc.lastGr.load();

        struct N { juce::String lab; int focus; float rms; };
        juce::Array<N> nodes;
        nodes.add ({ "In", In, inR });
        nodes.add ({ "Drive", Drive, inR });
        if (route == 3)
            for (int i = 0; i < bands; ++i)
                nodes.add ({ "L" + juce::String (i + 1), i, juce::jmax (inR, outR) });
        else if (route == 4)
        {
            nodes.add ({ "Mid", 0, inR });
            nodes.add ({ "Side", 1, outR });
        }
        else if (route == 1 || route == 2 || route == 5)
        {
            nodes.add ({ "Lens A", 0, inR });
            nodes.add ({ "Lens B", 1, outR });
        }
        else
            nodes.add ({ "Lens", 0, juce::jmax (inR, outR) });
        nodes.add ({ "Cavity", Cavity, amt });
        nodes.add ({ "Comp", Comp, 1.f - gr });
        nodes.add ({ "Clip", Clip, outR });
        nodes.add ({ "Out", Out, outR });

        auto row = r.reduced (6.f, 8.f);
        const float w = juce::jmax (40.f, (row.getWidth() - 8.f * (float) (nodes.size() - 1)) / (float) nodes.size());
        juce::Rectangle<float> prev;
        for (int i = 0; i < nodes.size(); ++i)
        {
            auto b = row.removeFromLeft (w).withSizeKeepingCentre (w - 2.f, 28.f);
            if (i > 0)
            {
                g.setColour (kEmerald.withAlpha (0.35f + 0.5f * nodes.getReference (i).rms));
                g.drawLine (prev.getRight(), prev.getCentreY(), b.getX(), b.getCentreY(),
                            1.2f + 3.f * nodes.getReference (i).rms);
            }
            float pulse = 1.f;
            if (nodes[i].focus == Cavity)
                pulse = 0.85f + 0.15f * std::sin ((float) proc.cavIndex.load() * 0.05f);
            const bool on = nodes[i].focus == selected;
            const float rmsA = juce::jlimit (0.f, 1.f, nodes[i].rms);
            g.setColour (on ? kEmerald : kEmerald.withAlpha (0.18f + 0.55f * rmsA));
            auto body = b.withSizeKeepingCentre (b.getWidth() * pulse,
                                                 b.getHeight() * (nodes[i].focus == Comp ? juce::jlimit (0.45f, 1.f, gr) : 1.f));
            g.fillRoundedRectangle (body, 4.f);
            if (on)
            {
                g.setColour (kSage);
                g.drawRoundedRectangle (b, 4.f, 1.2f);
            }
            g.setColour (on ? kWhite : kSage);
            g.setFont (11.f);
            g.drawText (nodes[i].lab, b.toNearestInt(), juce::Justification::centred, true);
            hits.add ({ b.toNearestInt(), nodes[i].focus });
            prev = b;
            row.removeFromLeft (8);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (auto& h : hits)
            if (h.area.contains (e.getPosition()))
            {
                selected = h.focus;
                if (h.focus >= 0)
                    proc.focusLens.store (h.focus);
                if (onFocus)
                    onFocus (h.focus);
                return;
            }
    }

    std::function<void (int)> onFocus;

    void timerCallback() override { repaint(); }

private:
    struct Hit { juce::Rectangle<int> area; int focus; };
    juce::Array<Hit> hits;
    fD1STAudioProcessor& proc;
};
