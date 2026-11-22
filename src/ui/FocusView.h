#pragma once

#include "../PluginProcessor.h"
#include "Theme.h"

#include <cmath>

class FocusView : public juce::Component, private juce::Timer
{
public:
    explicit FocusView (fD1STAudioProcessor& p, bool contextStrip = false)
        : proc (p), strip (contextStrip)
    {
        startTimerHz (60);
    }

    void paint (juce::Graphics& g) override
    {
        using namespace fd1st;
        auto r = getLocalBounds().toFloat();
        g.setColour (kWell);
        g.fillRoundedRectangle (r, 6.f);
        if (strip)
        {
            const float amt = proc.apvts.getRawParameterValue (ParamID::glob_fb_amt)->load();
            const int route = (int) proc.apvts.getRawParameterValue (ParamID::glob_route)->load();
            const float sp = proc.apvts.getRawParameterValue (ParamID::glob_spread)->load();
            const bool side = amt > 0.01f || route == 5 || route == 4 || std::abs (sp) > 0.05f;
            auto cavR = side ? r.removeFromRight (juce::jmin (160.f, r.getWidth() * 0.28f)).reduced (6.f)
                             : juce::Rectangle<float> {};
            drawSpectrum (g, r.reduced (6.f));
            if (side)
                drawCavity (g, cavR);
            return;
        }

        auto plot = r.removeFromLeft (r.getWidth() * 0.58f).reduced (8.f);
        auto bloomR = r.reduced (6.f);

        g.setColour (kBlack);
        g.fillRect (plot);
        const int lens = juce::jlimit (0, 5, proc.focusLens.load());
        const int style = (int) proc.apvts.getRawParameterValue (ParamID::cStyle[lens])->load();
        const float drive = proc.apvts.getRawParameterValue (ParamID::cDriveDb[lens])->load();
        const float heat = proc.apvts.getRawParameterValue (ParamID::glob_heat)->load();
        const float bias = proc.apvts.getRawParameterValue (ParamID::cBias[lens])->load();
        const float sr = 48000.f;
        const bool odd = (style % 2) == 0;
        juce::Path dry, wet;
        const int N = 512;
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
        g.setColour (odd ? kEmerald : kWhite.interpolatedWith (kEmerald, 0.4f));
        g.strokePath (wet, juce::PathStrokeType (1.6f));

        int pw = proc.probeW.load();
        float inHist[32] {}, outHist[32] {};
        for (int i = 0; i < 256; ++i)
        {
            const float xi = proc.probeIn[(size_t) ((pw - 1 - i + fD1STAudioProcessor::kProbeN) % fD1STAudioProcessor::kProbeN)];
            const float yi = proc.probeOut[(size_t) ((pw - 1 - i + fD1STAudioProcessor::kProbeN) % fD1STAudioProcessor::kProbeN)];
            int bx = juce::jlimit (0, 31, (int) ((xi * 0.5f + 0.5f) * 32.f));
            int by = juce::jlimit (0, 31, (int) ((yi * 0.5f + 0.5f) * 32.f));
            inHist[bx] += 1.f;
            outHist[by] += 1.f;
            if (i < 64)
            {
                const float xd = juce::jmap (juce::jlimit (-1.5f, 1.5f, xi), -1.5f, 1.5f, plot.getX(), plot.getRight());
                const float yw = juce::jmap (juce::jlimit (-1.5f, 1.5f, -yi), -1.5f, 1.5f, plot.getBottom(), plot.getY());
                g.setColour (kEmerald.withAlpha (0.15f + 0.5f * (1.f - (float) i / 64.f)));
                g.fillEllipse (xd - 2.f, yw - 2.f, 4.f, 4.f);
            }
        }
        g.setColour (kSage.withAlpha (0.35f));
        for (int i = 0; i < 32; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * ((float) i / 32.f);
            g.fillRect (x, plot.getBottom() - 18.f * (inHist[i] / 32.f), plot.getWidth() / 32.f, 18.f * (inHist[i] / 32.f));
        }

        drawBloom (g, bloomR);

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
    bool strip = false;

    void copyProbe (float* dest, int n, bool wet)
    {
        int pw = proc.probeW.load();
        for (int i = 0; i < n; ++i)
        {
            const int idx = (pw - n + i + fD1STAudioProcessor::kProbeN * 4) % fD1STAudioProcessor::kProbeN;
            dest[i] = wet ? proc.probeOut[(size_t) idx] : proc.probeIn[(size_t) idx];
        }
    }

    void drawBloom (juce::Graphics& g, juce::Rectangle<float> r)
    {
        using namespace fd1st;
        g.setColour (kBlack);
        g.fillRect (r);
        float buf[2048];
        copyProbe (buf, 2048, true);
        int lag = 40;
        float best = 0;
        for (int k = 40; k < 400; ++k)
        {
            float acc = 0;
            for (int i = 0; i < 1024; ++i)
                acc += buf[i] * buf[i + k];
            if (acc > best)
            {
                best = acc;
                lag = k;
            }
        }
        const float f0 = 48000.f / (float) lag;
        const auto c = r.getCentre();
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.42f;
        for (int h = 1; h <= 16; ++h)
        {
            float re = 0, im = 0;
            const float w = 6.2831853f * f0 * (float) h / 48000.f;
            for (int i = 0; i < 1024; ++i)
            {
                re += buf[i] * std::cos (w * (float) i);
                im += buf[i] * std::sin (w * (float) i);
            }
            const float mag = std::sqrt (re * re + im * im) / 512.f;
            const float a = ((float) h - 1.f) / 16.f * 6.2831853f;
            const float rr = rad * juce::jlimit (0.05f, 1.f, mag * 4.f);
            g.setColour ((h & 1) ? kEmerald.withAlpha (0.85f) : kWhite.withAlpha (0.7f));
            g.fillEllipse (c.x + std::cos (a) * rr - 3.f, c.y + std::sin (a) * rr - 3.f, 6.f, 6.f);
        }
        g.setColour (kSage);
        g.setFont (10.f);
        g.drawText ("Bloom", r, juce::Justification::topLeft);
    }

    void drawSpectrum (juce::Graphics& g, juce::Rectangle<float> r)
    {
        using namespace fd1st;
        g.setColour (kBlack);
        g.fillRect (r);
        float in[1024], out[1024];
        copyProbe (in, 1024, false);
        copyProbe (out, 1024, true);
        juce::Path pin, pout;
        const int bins = 128;
        for (int b = 0; b < bins; ++b)
        {
            const float t = (float) b / (float) (bins - 1);
            const float hz = 20.f * std::pow (1000.f, t);
            auto mag = [&] (float* x)
            {
                float re = 0, im = 0;
                const float w = 6.2831853f * hz / 48000.f;
                for (int i = 0; i < 512; ++i)
                {
                    re += x[i] * std::cos (w * (float) i);
                    im += x[i] * std::sin (w * (float) i);
                }
                return juce::Decibels::gainToDecibels (std::sqrt (re * re + im * im) / 256.f + 1.0e-6f);
            };
            const float xi = juce::jmap (t, r.getX(), r.getRight());
            const float yi = juce::jmap (juce::jlimit (-80.f, 0.f, mag (in)), -80.f, 0.f, r.getBottom(), r.getY());
            const float yo = juce::jmap (juce::jlimit (-80.f, 0.f, mag (out)), -80.f, 0.f, r.getBottom(), r.getY());
            if (b == 0)
            {
                pin.startNewSubPath (xi, yi);
                pout.startNewSubPath (xi, yo);
            }
            else
            {
                pin.lineTo (xi, yi);
                pout.lineTo (xi, yo);
            }
        }
        pin.lineTo (r.getRight(), r.getBottom());
        pin.lineTo (r.getX(), r.getBottom());
        pin.closeSubPath();
        g.setColour (kSage.withAlpha (0.3f));
        g.fillPath (pin);
        g.setColour (kEmerald);
        g.strokePath (pout, juce::PathStrokeType (1.2f));
        const int route = (int) proc.apvts.getRawParameterValue (ParamID::glob_route)->load();
        if (route == 3)
        {
            const int bands = juce::jlimit (1, 6, (int) proc.apvts.getRawParameterValue (ParamID::glob_bands)->load());
            for (int i = 0; i < bands - 1; ++i)
            {
                const float hz = proc.apvts.getRawParameterValue (ParamID::xoverHz[i])->load();
                const float t = std::log (hz / 20.f) / std::log (1000.f);
                const float x = juce::jmap (juce::jlimit (0.f, 1.f, t), r.getX(), r.getRight());
                g.setColour (kWhite.withAlpha (0.35f));
                g.drawLine (x, r.getY(), x, r.getBottom(), 1.f);
            }
        }
        g.setColour (kSage);
        g.setFont (10.f);
        g.drawText ("Spectrum", r, juce::Justification::topLeft);
    }

    void drawCavity (juce::Graphics& g, juce::Rectangle<float> r)
    {
        using namespace fd1st;
        const float amt = proc.apvts.getRawParameterValue (ParamID::glob_fb_amt)->load();
        const int route = (int) proc.apvts.getRawParameterValue (ParamID::glob_route)->load();
        if (amt < 0.01f && route != 5)
        {
            const float sp = proc.apvts.getRawParameterValue (ParamID::glob_spread)->load();
            if (route == 4 || std::abs (sp) > 0.05f)
            {
                g.setColour (kBlack);
                g.fillRect (r);
                g.setColour (kEmerald);
                g.drawEllipse (r.reduced (8.f), 1.f);
                g.setColour (kSage);
                g.setFont (10.f);
                g.drawText ("Vectors", r, juce::Justification::centred);
            }
            return;
        }
        g.setColour (kBlack);
        g.fillEllipse (r.reduced (4.f));
        const float thick = 2.f + amt * 10.f;
        g.setColour (kEmerald);
        g.drawEllipse (r.reduced (8.f), thick);
        const float ang = (float) (proc.cavIndex.load() % 360) * 0.017453f;
        const auto c = r.getCentre();
        g.setColour (kWhite);
        g.fillEllipse (c.x + std::cos (ang) * r.getWidth() * 0.35f - 3.f,
                       c.y + std::sin (ang) * r.getHeight() * 0.35f - 3.f, 6.f, 6.f);
        g.setColour (kSage);
        g.setFont (10.f);
        g.drawText ("Cavity", r, juce::Justification::centred);
    }
};
