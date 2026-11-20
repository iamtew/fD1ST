#pragma once

#include "PluginProcessor.h"
#include "ui/FocusView.h"
#include "ui/PathView.h"
#include "ui/Theme.h"

class fD1STAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit fD1STAudioProcessorEditor (fD1STAudioProcessor&);
    ~fD1STAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    struct Content : public juce::Component
    {
        explicit Content (fD1STAudioProcessorEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
        void resized() override { ed.layoutContent(); }
        fD1STAudioProcessorEditor& ed;
    };

    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        juce::Font getLabelFont (juce::Label&) override;
    };

    struct Knob
    {
        juce::Slider s;
        juce::Label l;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> a;
    };

    void paintContent (juce::Graphics&);
    void layoutContent();
    void bindKnob (Knob& k, const char* id, const juce::String& name);
    void setListen (int id);

    fD1STAudioProcessor& proc;
    PanelLnF panelLnF;
    Content content { *this };
    int baseW = 0, baseH = 0;
    juce::OpenGLContext gl;

    juce::Image logo;
    PathView path { proc };
    FocusView focus { proc };
    Knob mix, out, globDrive, drive, heat, spread, motionAmt, bias;
    juce::ComboBox osBox, styleBox, routeBox, destBox;
    juce::Label osLabel, styleLabel, routeLabel, gpuBanner, wireLabel;
    juce::Slider wireDepth;
    juce::ToggleButton bypass, wireOn;
    juce::TextButton listenW, listenD, listenE, listenC;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAtt, styleAtt, routeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt;
    juce::TooltipWindow tip { this, 400 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fD1STAudioProcessorEditor)
};
