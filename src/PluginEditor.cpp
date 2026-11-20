#include "PluginEditor.h"

namespace
{
constexpr int kEditorW = 520;
constexpr int kEditorH = 280;
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
    bindKnob (drive, ParamID::cDriveDb[0], "Lens 0 Drive");

    bypass.setButtonText ("Bypass");
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::glob_bypass, bypass);
    content.addAndMakeVisible (bypass);

    addAndMakeVisible (content);
    baseW = kEditorW;
    baseH = kEditorH;
    setSize (baseW, baseH);
    setResizable (true, true);
    setResizeLimits (baseW / 2, baseH / 2, baseW * 3, baseH * 3);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
}

fD1STAudioProcessorEditor::~fD1STAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
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
    r.removeFromTop (8);
    const int col = r.getWidth() / 3;
    auto place = [&] (Knob& k, juce::Rectangle<int> cell)
    {
        k.l.setBounds (cell.removeFromTop (16));
        k.s.setBounds (cell);
    };
    place (mix, r.removeFromLeft (col).reduced (8, 0));
    place (out, r.removeFromLeft (col).reduced (8, 0));
    place (drive, r.reduced (8, 0));
}
