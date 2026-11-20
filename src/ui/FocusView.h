#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

class FocusView : public juce::Component, private juce::Timer
{
public:
    explicit FocusView (fD1STAudioProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        using namespace fd1st;
        auto r = getLocalBounds().toFloat();
        g.setColour (kWell);
        g.fillRoundedRectangle (r, 6.f);
        auto plot = r.reduced (8.f);
        g.setColour (kBlack);
        g.fillRect (plot);

        const int style = (int) proc.apvts.getRawParameterValue (ParamID::cStyle[0])->load();
        const float drive = proc.apvts.getRawParameterValue (ParamID::cDriveDb[0])->load();
        const float heat = proc.apvts.getRawParameterValue (ParamID::glob_heat)->load();
        const float bias = proc.apvts.getRawParameterValue (ParamID::cBias[0])->load();
        const float sr = 48000.f;

        juce::Path dry, wet;
        const int N = 256;
        for (int i = 0; i < N; ++i)
        {
            const float x = -1.5f + 3.f * (float) i / (float) (N - 1);
            const float xd = juce::jmap (x, -1.5f, 1.5f, plot.getX(), plot.getRight());
            const float yDry = juce::jmap (-x, -1.5f, 1.5f, plot.getBottom(), plot.getY());
            const float y = fd1st::LensProcessor::transfer (style, x, drive, heat, bias, sr);
            const float yw = juce::jmap (juce::jlimit (-1.5f, 1.5f, -y), -1.5f, 1.5f, plot.getBottom(), plot.getY());
            if (i == 0)
            {
                dry.startNewSubPath (xd, yDry);
                wet.startNewSubPath (xd, yw);
            }
            else
            {
                dry.lineTo (xd, yDry);
                wet.lineTo (xd, yw);
            }
        }
        g.setColour (kSage.withAlpha (0.55f));
        g.strokePath (dry, juce::PathStrokeType (1.f));
        g.setColour (kEmerald);
        g.strokePath (wet, juce::PathStrokeType (1.6f));

        const float inM = proc.inPeak.load();
        const float outM = proc.outPeak.load();
        auto meters = plot.removeFromRight (10);
        g.setColour (kSage.withAlpha (0.8f));
        g.fillRect (meters.removeFromLeft (4).removeFromBottom (meters.getHeight() * juce::jlimit (0.f, 1.f, inM)));
        g.setColour (kEmerald);
        g.fillRect (meters.removeFromBottom (meters.getHeight() * juce::jlimit (0.f, 1.f, outM)));
    }

    void timerCallback() override { repaint(); }

private:
    fD1STAudioProcessor& proc;
};
