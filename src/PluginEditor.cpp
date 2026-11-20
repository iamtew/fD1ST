#include "PluginEditor.h"

namespace
{
constexpr int kEditorW = 1100;
constexpr int kEditorH = 720;
constexpr int kPad = 10;
constexpr int kTitleH = 40;
}

void fD1STAudioProcessorEditor::PanelLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                            float pos, float startAngle, float endAngle,
                                                            juce::Slider&)
{
    using namespace fd1st;
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    if (d < 8.f)
        return;
    auto area = bounds.withSizeKeepingCentre (d, d);
    const auto c = area.getCentre();
    g.setColour (kSage);
    for (int i = 0; i < 11; ++i)
    {
        const float t = (float) i / 10.f;
        const float a = startAngle + t * (endAngle - startAngle);
        const auto p0 = c.getPointOnCircumference (d * 0.48f, a);
        const auto p1 = c.getPointOnCircumference (d * 0.40f, a);
        g.drawLine (p0.x, p0.y, p1.x, p1.y, juce::jmax (1.f, d * 0.018f));
    }
    const float bodyD = d * 0.72f;
    auto body = area.withSizeKeepingCentre (bodyD, bodyD);
    g.setColour (kBlack.withAlpha (0.45f));
    g.fillEllipse (body.translated (0.f, d * 0.018f));
    g.setColour (kForest);
    g.fillEllipse (body);
    auto cap = area.withSizeKeepingCentre (bodyD * 0.5f, bodyD * 0.5f);
    g.setColour (kEmerald);
    g.fillEllipse (cap);
    const float angle = startAngle + pos * (endAngle - startAngle);
    juce::Path pointer;
    pointer.startNewSubPath (c.getPointOnCircumference (bodyD * 0.32f, angle));
    pointer.lineTo (c.getPointOnCircumference (bodyD * 0.46f, angle));
    g.setColour (kWhite);
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (1.15f, d * 0.028f),
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

void fD1STAudioProcessorEditor::PanelLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b,
                                                            bool, bool)
{
    using namespace fd1st;
    auto r = b.getLocalBounds().toFloat().reduced (2.f);
    g.setColour (b.getToggleState() ? kEmerald : kForest);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (kSage);
    g.setFont (12.f);
    g.drawFittedText (b.getButtonText(), r.toNearestInt(), juce::Justification::centred, 1);
}

juce::Font fD1STAudioProcessorEditor::PanelLnF::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (11.f));
}

void fD1STAudioProcessorEditor::bindKnob (Knob& k, const char* id, const juce::String& name)
{
    using namespace fd1st;
    k.s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 16);
    k.s.setColour (juce::Slider::textBoxTextColourId, kWhite);
    k.s.setColour (juce::Slider::textBoxBackgroundColourId, kWell);
    k.s.setColour (juce::Slider::textBoxOutlineColourId, kForest);
    k.l.setText (name, juce::dontSendNotification);
    k.l.setJustificationType (juce::Justification::centred);
    k.l.setColour (juce::Label::textColourId, kSage);
    k.a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.s);
    content.addAndMakeVisible (k.s);
    content.addAndMakeVisible (k.l);
}

fD1STAudioProcessorEditor::fD1STAudioProcessorEditor (fD1STAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    using namespace fd1st;
    setLookAndFeel (&panelLnF);
    setOpaque (true);
    content.setOpaque (true);

    panelLnF.setColour (juce::TooltipWindow::backgroundColourId, kWell);
    panelLnF.setColour (juce::TooltipWindow::textColourId, kSage);
    panelLnF.setColour (juce::TooltipWindow::outlineColourId, kEmerald);

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    bindKnob (mix, ParamID::glob_mix, "Mix");
    bindKnob (out, ParamID::glob_out_db, "Output");
    bindKnob (globDrive, ParamID::glob_drive_db, "Drive");
    bindKnob (drive, ParamID::cDriveDb[0], "Lens Drive");
    bindKnob (heat, ParamID::glob_heat, "Heat");
    bindKnob (spread, ParamID::glob_spread, "Spread");
    bindKnob (motionAmt, ParamID::glob_motion, "Motion");
    bindKnob (bias, ParamID::cBias[0], "Bias");

    auto colourCombo = [&] (juce::ComboBox& b)
    {
        b.setColour (juce::ComboBox::backgroundColourId, kWell);
        b.setColour (juce::ComboBox::textColourId, kWhite);
        b.setColour (juce::ComboBox::outlineColourId, kForest);
        b.setColour (juce::ComboBox::arrowColourId, kEmerald);
    };
    colourCombo (osBox);
    colourCombo (styleBox);
    colourCombo (routeBox);
    colourCombo (destBox);
    osLabel.setText ("OS", juce::dontSendNotification);
    styleLabel.setText ("Style", juce::dontSendNotification);
    routeLabel.setText ("Path", juce::dontSendNotification);
    for (auto* l : { &osLabel, &styleLabel, &routeLabel, &wireLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, kSage);
        content.addAndMakeVisible (*l);
    }
    osAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::glob_os, osBox);
    styleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::cStyle[0], styleBox);
    routeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::glob_route, routeBox);
    destBox.addItem (ParamID::glob_mix, 1);
    destBox.setSelectedId (1, juce::dontSendNotification);
    destBox.onChange = [this]
    {
        proc.motion.tree.getChildWithName ("wire0").setProperty ("dst", destBox.getText(), nullptr);
    };
    wireLabel.setText ("Wiring", juce::dontSendNotification);
    wireDepth.setRange (-1.0, 1.0, 0.01);
    wireDepth.onValueChange = [this]
    {
        proc.motion.tree.getChildWithName ("wire0").setProperty ("depth", wireDepth.getValue(), nullptr);
    };
    wireOn.setButtonText ("Orbit->Mix");
    wireOn.onClick = [this]
    {
        auto w = proc.motion.tree.getChildWithName ("wire0");
        w.setProperty ("on", wireOn.getToggleState(), nullptr);
        w.setProperty ("dst", ParamID::glob_mix, nullptr);
    };
    content.addAndMakeVisible (osBox);
    content.addAndMakeVisible (styleBox);
    content.addAndMakeVisible (routeBox);
    content.addAndMakeVisible (destBox);
    content.addAndMakeVisible (wireDepth);
    content.addAndMakeVisible (wireOn);

    listenW.setButtonText ("W");
    listenD.setButtonText ("Dry");
    listenE.setButtonText ("E");
    listenC.setButtonText ("C");
    listenW.onClick = [this] { setListen (0); };
    listenD.onClick = [this] { setListen (1); };
    listenE.onClick = [this] { setListen (2); };
    listenC.onClick = [this] { setListen (3); };
    content.addAndMakeVisible (listenW);
    content.addAndMakeVisible (listenD);
    content.addAndMakeVisible (listenE);
    content.addAndMakeVisible (listenC);

    gpuBanner.setText ("GPU off — reduced sight", juce::dontSendNotification);
    gpuBanner.setColour (juce::Label::textColourId, kSage);
    gpuBanner.setVisible (false);
    content.addAndMakeVisible (gpuBanner);

    bypass.setButtonText ("Bypass");
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::glob_bypass, bypass);
    content.addAndMakeVisible (bypass);
    content.addAndMakeVisible (path);
    content.addAndMakeVisible (focus);

    addAndMakeVisible (content);
    gl.attachTo (*this);
    setWantsKeyboardFocus (true);
    baseW = kEditorW;
    baseH = kEditorH;
    setSize (baseW, baseH);
    setResizable (true, true);
    setResizeLimits (900, 560, baseW * 2, baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
}

fD1STAudioProcessorEditor::~fD1STAudioProcessorEditor()
{
    gl.detach();
    setLookAndFeel (nullptr);
}

void fD1STAudioProcessorEditor::setListen (int id)
{
    if (auto* p = proc.apvts.getParameter (ParamID::glob_listen))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) id));
}

bool fD1STAudioProcessorEditor::keyPressed (const juce::KeyPress& k)
{
    if (k.getTextCharacter() == 'w' || k.getTextCharacter() == 'W')
    {
        setListen (0);
        return true;
    }
    if (k.getTextCharacter() == 'd' && k.getModifiers().isShiftDown())
    {
        setListen (1);
        return true;
    }
    if (k.getTextCharacter() == 'e' || k.getTextCharacter() == 'E')
    {
        setListen (2);
        return true;
    }
    if (k.getTextCharacter() == 'c' || k.getTextCharacter() == 'C')
    {
        setListen (3);
        return true;
    }
    return false;
}

void fD1STAudioProcessorEditor::paint (juce::Graphics&) {}

void fD1STAudioProcessorEditor::resized()
{
    if (baseW == 0)
        return;
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, baseW, baseH);
}

void fD1STAudioProcessorEditor::paintContent (juce::Graphics& g)
{
    using namespace fd1st;
    g.fillAll (kMain);

    auto r = content.getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    if (logo.isValid())
    {
        auto logoR = title.removeFromLeft (kTitleH).reduced (2);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
        title.removeFromLeft (6);
    }
    g.setColour (kSage);
    g.setFont (boldFont (18.f));
    g.drawText ("fD1ST", title, juce::Justification::centredLeft, true);
    g.setFont (juce::Font (juce::FontOptions (12.f)));
    g.drawText ("Watch the signal break.", title, juce::Justification::centredRight, true);
}

void fD1STAudioProcessorEditor::layoutContent()
{
    auto r = content.getLocalBounds().reduced (kPad);
    r.removeFromTop (kTitleH + 8);
    auto bar = r.removeFromTop (28);
    bypass.setBounds (bar.removeFromRight (88));
    auto osArea = bar.removeFromRight (120);
    osLabel.setBounds (osArea.removeFromLeft (28));
    osBox.setBounds (osArea);
    listenC.setBounds (bar.removeFromRight (36));
    listenE.setBounds (bar.removeFromRight (36));
    listenD.setBounds (bar.removeFromRight (44));
    listenW.setBounds (bar.removeFromRight (36));
    routeLabel.setBounds (bar.removeFromLeft (36));
    routeBox.setBounds (bar.removeFromLeft (100));
    gpuBanner.setBounds (bar);
    r.removeFromTop (8);
    path.setBounds (r.removeFromTop (72));
    r.removeFromTop (8);
    auto focusR = r.removeFromTop (280);
    focus.setBounds (focusR.removeFromLeft (360));
    auto knobs = focusR;
    auto row1 = knobs.removeFromTop (120);
    const int col = row1.getWidth() / 4;
    auto place = [&] (Knob& k, juce::Rectangle<int> cell)
    {
        k.l.setBounds (cell.removeFromTop (16));
        k.s.setBounds (cell);
    };
    place (mix, row1.removeFromLeft (col).reduced (4, 0));
    place (out, row1.removeFromLeft (col).reduced (4, 0));
    place (globDrive, row1.removeFromLeft (col).reduced (4, 0));
    place (drive, row1.reduced (4, 0));
    auto row2 = knobs.removeFromTop (120);
    const int col2 = row2.getWidth() / 4;
    place (heat, row2.removeFromLeft (col2).reduced (4, 0));
    place (spread, row2.removeFromLeft (col2).reduced (4, 0));
    place (motionAmt, row2.removeFromLeft (col2).reduced (4, 0));
    place (bias, row2.reduced (4, 0));
    auto row3 = knobs.removeFromTop (28);
    styleLabel.setBounds (row3.removeFromLeft (48));
    styleBox.setBounds (row3.removeFromLeft (160));
    r.removeFromTop (8);
    auto motionRow = r.removeFromTop (28);
    wireLabel.setBounds (motionRow.removeFromLeft (56));
    wireOn.setBounds (motionRow.removeFromLeft (100));
    destBox.setBounds (motionRow.removeFromLeft (120));
    wireDepth.setBounds (motionRow);
}
