#pragma once

#include "PluginProcessor.h"
#include "ui/FocusView.h"
#include "ui/PathView.h"
#include "ui/Theme.h"

class fD1STAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  public juce::DragAndDropContainer
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
    void hideKnob (Knob& k);
    void placeKnob (Knob& k, juce::Rectangle<int> cell);
    void hideStageControls();
    void setListen (int id);
    void rebindLens();
    void pushUndo();
    void doUndo();
    void doRedo();
    void copyAtoB();
    void morphAB();

    fD1STAudioProcessor& proc;
    PanelLnF panelLnF;
    Content content { *this };
    int baseW = 0, baseH = 0;
    juce::OpenGLContext gl;
    bool gpuOk = false;

    juce::Image logo;
    int logoDrawW = 0;
    int focusStage = PathView::Drive;
    PathView path { proc };
    FocusView focus { proc };
    FocusView context { proc, true };
    Knob mix, out, globDrive, drive, heat, spread, motionAmt, bias;
    Knob blend, tone, cavTime, cavAmt, cavNote, comp, lensMix, lensDyn, lensLevel, lensPan;
    Knob lensFb, lensFbHz, filtHz, filtQ, bass, mid, treble, presence, xover1, xover2, xover3, abMorph;
    juce::ComboBox osBox, styleBox, routeBox, destBox, programBox, chmodeBox, fbModeBox, filtBox, stampBox, srcBox;
    juce::Label osLabel, styleLabel, routeLabel, gpuBanner, wireLabel, progLabel;
    juce::Slider wireDepth;
    juce::ToggleButton bypass, wireOn, linphase, colorcomp, cavSafe, lensOn, lensMute, lensSolo, midiLearn, helpBtn;
    juce::TextButton listenW, listenD, listenE, listenC, undoBtn, redoBtn, abA, abB, fullBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osAtt, styleAtt, routeAtt, chmodeAtt, fbModeAtt, filtAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt, linAtt, colorAtt, safeAtt, onAtt, muteAtt, soloAtt;
    juce::TooltipWindow tip { this, 400 };
    juce::Label helpText;
    juce::Label stageHint;

    juce::Array<juce::MemoryBlock> undoStack;
    int undoPos = -1;
    juce::MemoryBlock snapA, snapB;
    bool showingB = false;
    bool learning = false;
    juce::String lastParam;
    bool showHelp = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fD1STAudioProcessorEditor)
};
