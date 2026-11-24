#include "PluginEditor.h"

fD1STAudioProcessorEditor::fD1STAudioProcessorEditor (fD1STAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setSize (480, 280);
    setResizable (false, false);

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    title.setText ("fD1ST", juce::dontSendNotification);
    title.setFont (fd1st::boldFont (28.f));
    title.setColour (juce::Label::textColourId, fd1st::kSage);
    title.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (title);

    spoken.setText ("DIST", juce::dontSendNotification);
    spoken.setFont (fd1st::boldFont (14.f));
    spoken.setColour (juce::Label::textColourId, fd1st::kSage.withAlpha (0.8f));
    spoken.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (spoken);

    out.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    out.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
    out.setColour (juce::Slider::rotarySliderFillColourId, fd1st::kEmerald);
    out.setColour (juce::Slider::rotarySliderOutlineColourId, fd1st::kForest);
    out.setColour (juce::Slider::thumbColourId, fd1st::kWhite);
    out.setColour (juce::Slider::textBoxTextColourId, fd1st::kWhite);
    out.setColour (juce::Slider::textBoxOutlineColourId, fd1st::kForest);
    addAndMakeVisible (out);
    outAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, ParamID::out_db, out);

    outLabel.setText ("Out", juce::dontSendNotification);
    outLabel.setFont (fd1st::boldFont (12.f));
    outLabel.setColour (juce::Label::textColourId, fd1st::kSage);
    outLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (outLabel);
}

void fD1STAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (fd1st::kMain);
    if (logo.isValid())
        g.drawImageWithin (logo, 24, 24, 72, 72, juce::RectanglePlacement::centred);
}

void fD1STAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (24);
    auto head = r.removeFromTop (72);
    head.removeFromLeft (88);
    title.setBounds (head.removeFromTop (36));
    spoken.setBounds (head.removeFromTop (24));
    r.removeFromTop (16);
    auto knob = r.removeFromLeft (120);
    out.setBounds (knob.removeFromTop (100));
    outLabel.setBounds (knob.removeFromTop (20));
}
