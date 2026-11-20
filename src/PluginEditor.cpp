#include "PluginEditor.h"
#include "presets/Factory.h"

namespace
{
constexpr int kEditorW = 1100;
constexpr int kEditorH = 720;
constexpr int kPad = 10;
constexpr int kTitleH = 36;
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
    k.s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 48, 14);
    k.s.setColour (juce::Slider::textBoxTextColourId, kWhite);
    k.s.setColour (juce::Slider::textBoxBackgroundColourId, kWell);
    k.s.setColour (juce::Slider::textBoxOutlineColourId, kForest);
    k.l.setText (name, juce::dontSendNotification);
    k.l.setJustificationType (juce::Justification::centred);
    k.l.setColour (juce::Label::textColourId, kSage);
    k.a.reset();
    k.a = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.s);
    content.addAndMakeVisible (k.s);
    content.addAndMakeVisible (k.l);
    k.s.onDragStart = [this, id] { lastParam = id; pushUndo(); };
}

void fD1STAudioProcessorEditor::rebindLens()
{
    const int i = juce::jlimit (0, 5, proc.focusLens.load());
    bindKnob (drive, ParamID::cDriveDb[i], "Drive");
    bindKnob (bias, ParamID::cBias[i], "Bias");
    bindKnob (lensMix, ParamID::cMix[i], "Lens Mix");
    bindKnob (lensDyn, ParamID::cDyn[i], "Dyn");
    bindKnob (lensLevel, ParamID::cLevelDb[i], "Level");
    bindKnob (lensPan, ParamID::cPan[i], "Pan");
    bindKnob (lensFb, ParamID::cFbAmt[i], "Ring");
    bindKnob (lensFbHz, ParamID::cFbHz[i], "Ring Hz");
    bindKnob (filtHz, ParamID::cFiltHz[i], "Filt Hz");
    bindKnob (filtQ, ParamID::cFiltQ[i], "Q");
    bindKnob (bass, ParamID::cBass[i], "Bass");
    bindKnob (mid, ParamID::cMid[i], "Mid");
    bindKnob (treble, ParamID::cTreble[i], "Treble");
    bindKnob (presence, ParamID::cPresence[i], "Pres");
    styleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::cStyle[i], styleBox);
    filtAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::cFiltType[i], filtBox);
    onAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::cOn[i], lensOn);
    muteAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::cMute[i], lensMute);
    soloAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::cSolo[i], lensSolo);
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
    bindKnob (heat, ParamID::glob_heat, "Heat");
    bindKnob (spread, ParamID::glob_spread, "Spread");
    bindKnob (motionAmt, ParamID::glob_motion, "Motion");
    bindKnob (blend, ParamID::glob_blend, "Blend");
    bindKnob (tone, ParamID::glob_tone, "Tone");
    bindKnob (cavTime, ParamID::glob_fb_time, "Time");
    bindKnob (cavAmt, ParamID::glob_fb_amt, "Cavity");
    bindKnob (cavNote, ParamID::glob_fb_note_hz, "Note");
    bindKnob (comp, ParamID::glob_comp, "Comp");
    bindKnob (xover1, ParamID::xoverHz[0], "X1");
    bindKnob (xover2, ParamID::xoverHz[1], "X2");
    bindKnob (xover3, ParamID::xoverHz[2], "X3");
    abMorph.s.setRange (0, 1, 0.001);
    abMorph.s.setSliderStyle (juce::Slider::LinearHorizontal);
    abMorph.l.setText ("A/B", juce::dontSendNotification);
    abMorph.s.onValueChange = [this] { morphAB(); };
    content.addAndMakeVisible (abMorph.s);
    content.addAndMakeVisible (abMorph.l);

    auto colourCombo = [&] (juce::ComboBox& b)
    {
        b.setColour (juce::ComboBox::backgroundColourId, kWell);
        b.setColour (juce::ComboBox::textColourId, kWhite);
        b.setColour (juce::ComboBox::outlineColourId, kForest);
        b.setColour (juce::ComboBox::arrowColourId, kEmerald);
        content.addAndMakeVisible (b);
    };
    colourCombo (osBox);
    colourCombo (styleBox);
    colourCombo (routeBox);
    colourCombo (destBox);
    colourCombo (programBox);
    colourCombo (chmodeBox);
    colourCombo (fbModeBox);
    colourCombo (filtBox);
    colourCombo (stampBox);
    colourCombo (srcBox);
    for (int i = 0; i < fd1st::kFactoryCount; ++i)
        programBox.addItem (fd1st::factoryName (i), i + 1);
    programBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    programBox.onChange = [this]
    {
        pushUndo();
        proc.setCurrentProgram (programBox.getSelectedItemIndex());
    };
    for (int i = 0; i < 12; ++i)
        stampBox.addItem (fd1st::stampName (i), i + 1);
    stampBox.onChange = [this]
    {
        fd1st::applyStamp (proc.apvts, proc.focusLens.load(), stampBox.getSelectedItemIndex());
    };
    srcBox.addItem ("orb:0", 1);
    srcBox.addItem ("orb:1", 2);
    srcBox.addItem ("stk:0", 3);
    srcBox.addItem ("fol:0", 4);
    srcBox.addItem ("nz:0", 5);
    srcBox.addItem ("xy:0", 6);
    srcBox.addItem ("mid:0", 7);
    srcBox.setSelectedId (1);
    destBox.addItem (ParamID::glob_mix, 1);
    destBox.addItem (ParamID::glob_heat, 2);
    destBox.addItem (ParamID::glob_spread, 3);
    destBox.addItem (ParamID::cDriveDb[0], 4);
    destBox.addItem (ParamID::cBias[0], 5);
    destBox.setSelectedId (1);

    osLabel.setText ("OS", juce::dontSendNotification);
    styleLabel.setText ("Style", juce::dontSendNotification);
    routeLabel.setText ("Path", juce::dontSendNotification);
    progLabel.setText ("Preset", juce::dontSendNotification);
    wireLabel.setText ("Wiring", juce::dontSendNotification);
    for (auto* l : { &osLabel, &styleLabel, &routeLabel, &wireLabel, &progLabel, &abMorph.l })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::textColourId, kSage);
        content.addAndMakeVisible (*l);
    }
    osAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::glob_os, osBox);
    routeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::glob_route, routeBox);
    chmodeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::glob_chmode, chmodeBox);
    fbModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        proc.apvts, ParamID::glob_fb_mode, fbModeBox);

    destBox.onChange = [this]
    {
        proc.motion.tree.getChildWithName ("wire0").setProperty ("dst", destBox.getText(), nullptr);
    };
    wireDepth.setRange (-1.0, 1.0, 0.01);
    wireDepth.onValueChange = [this]
    {
        proc.motion.tree.getChildWithName ("wire0").setProperty ("depth", wireDepth.getValue(), nullptr);
    };
    wireOn.setButtonText ("Wire");
    wireOn.onClick = [this]
    {
        auto w = proc.motion.tree.getChildWithName ("wire0");
        w.setProperty ("on", wireOn.getToggleState(), nullptr);
        w.setProperty ("src", srcBox.getText(), nullptr);
        w.setProperty ("dst", destBox.getText(), nullptr);
    };
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
    undoBtn.setButtonText ("Undo");
    redoBtn.setButtonText ("Redo");
    abA.setButtonText ("A");
    abB.setButtonText ("B");
    fullBtn.setButtonText ("Full");
    undoBtn.onClick = [this] { doUndo(); };
    redoBtn.onClick = [this] { doRedo(); };
    abA.onClick = [this]
    {
        showingB = false;
        proc.getStateInformation (snapA);
    };
    abB.onClick = [this]
    {
        showingB = true;
        if (snapB.getSize() > 0)
            proc.setStateInformation (snapB.getData(), (int) snapB.getSize());
        else
            copyAtoB();
    };
    fullBtn.onClick = [this]
    {
        if (auto* w = findParentComponentOfClass<juce::ResizableWindow>())
            w->setFullScreen (! w->isFullScreen());
    };
    for (auto* b : { &listenW, &listenD, &listenE, &listenC, &undoBtn, &redoBtn, &abA, &abB, &fullBtn })
        content.addAndMakeVisible (*b);

    gpuBanner.setText ("GPU off — reduced sight", juce::dontSendNotification);
    gpuBanner.setColour (juce::Label::textColourId, kSage);
    content.addAndMakeVisible (gpuBanner);

    bypass.setButtonText ("Bypass");
    linphase.setButtonText ("Lin");
    colorcomp.setButtonText ("Color");
    cavSafe.setButtonText ("Safe");
    lensOn.setButtonText ("On");
    lensMute.setButtonText ("Mute");
    lensSolo.setButtonText ("Solo");
    midiLearn.setButtonText ("MIDI");
    helpBtn.setButtonText ("?");
    midiLearn.onClick = [this]
    {
        learning = midiLearn.getToggleState();
        proc.midiLearnOn.store (learning);
        proc.midiLearnId = lastParam;
    };
    helpBtn.onClick = [this]
    {
        showHelp = helpBtn.getToggleState();
        helpText.setVisible (showHelp);
    };
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::glob_bypass, bypass);
    linAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::glob_linphase, linphase);
    colorAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::glob_colorcomp, colorcomp);
    safeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        proc.apvts, ParamID::glob_fb_safe, cavSafe);
    for (auto* b : { &bypass, &linphase, &colorcomp, &cavSafe, &lensOn, &lensMute, &lensSolo, &midiLearn, &helpBtn })
        content.addAndMakeVisible (*b);

    helpText.setText ("DIST: Path, Focus, Motion. W wet, Shift+D dry, E delta, C caustic. Drag wire src onto Mix.",
                      juce::dontSendNotification);
    helpText.setColour (juce::Label::textColourId, kSage);
    helpText.setVisible (false);
    content.addAndMakeVisible (helpText);

    path.onFocus = [this] (int) { rebindLens(); };
    content.addAndMakeVisible (path);
    content.addAndMakeVisible (focus);
    rebindLens();

    addAndMakeVisible (content);
    gl.attachTo (*this);
    gpuOk = true;
    gpuBanner.setVisible (false);
    gl.setContinuousRepainting (true);
    setWantsKeyboardFocus (true);
    baseW = kEditorW;
    baseH = kEditorH;
    setSize (baseW, baseH);
    setResizable (true, true);
    setResizeLimits (900, 560, baseW * 2, baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    proc.getStateInformation (snapA);
    pushUndo();
}

fD1STAudioProcessorEditor::~fD1STAudioProcessorEditor()
{
    gl.detach();
    setLookAndFeel (nullptr);
}

void fD1STAudioProcessorEditor::pushUndo()
{
    juce::MemoryBlock mb;
    proc.getStateInformation (mb);
    if (undoPos + 1 < undoStack.size())
        undoStack.removeRange (undoPos + 1, undoStack.size() - undoPos - 1);
    undoStack.add (mb);
    if (undoStack.size() > 50)
        undoStack.remove (0);
    undoPos = undoStack.size() - 1;
}

void fD1STAudioProcessorEditor::doUndo()
{
    if (undoPos <= 0)
        return;
    --undoPos;
    auto& mb = undoStack.getReference (undoPos);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    rebindLens();
}

void fD1STAudioProcessorEditor::doRedo()
{
    if (undoPos + 1 >= undoStack.size())
        return;
    ++undoPos;
    auto& mb = undoStack.getReference (undoPos);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    rebindLens();
}

void fD1STAudioProcessorEditor::copyAtoB()
{
    snapB = snapA;
}

void fD1STAudioProcessorEditor::morphAB()
{
    if (snapA.getSize() == 0 || snapB.getSize() == 0)
        return;
    const float t = (float) abMorph.s.getValue();
    if (t < 0.001f)
    {
        proc.setStateInformation (snapA.getData(), (int) snapA.getSize());
        return;
    }
    if (t > 0.999f)
    {
        proc.setStateInformation (snapB.getData(), (int) snapB.getSize());
        return;
    }
    proc.setStateInformation (t < 0.5f ? snapA.getData() : snapB.getData(),
                              t < 0.5f ? (int) snapA.getSize() : (int) snapB.getSize());
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
    if (k == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0))
    {
        doUndo();
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
    r.removeFromTop (kTitleH + 4);
    auto bar = r.removeFromTop (24);
    fullBtn.setBounds (bar.removeFromRight (48));
    helpBtn.setBounds (bar.removeFromRight (28));
    midiLearn.setBounds (bar.removeFromRight (52));
    bypass.setBounds (bar.removeFromRight (70));
    linphase.setBounds (bar.removeFromRight (40));
    auto osArea = bar.removeFromRight (110);
    osLabel.setBounds (osArea.removeFromLeft (22));
    osBox.setBounds (osArea);
    listenC.setBounds (bar.removeFromRight (32));
    listenE.setBounds (bar.removeFromRight (32));
    listenD.setBounds (bar.removeFromRight (40));
    listenW.setBounds (bar.removeFromRight (32));
    heat.s.setBounds ({}); // placed below
    progLabel.setBounds (bar.removeFromLeft (44));
    programBox.setBounds (bar.removeFromLeft (140));
    undoBtn.setBounds (bar.removeFromLeft (48));
    redoBtn.setBounds (bar.removeFromLeft (48));
    abA.setBounds (bar.removeFromLeft (28));
    abB.setBounds (bar.removeFromLeft (28));
    abMorph.l.setBounds (bar.removeFromLeft (28));
    abMorph.s.setBounds (bar.removeFromLeft (80));
    gpuBanner.setBounds (bar);

    r.removeFromTop (6);
    path.setBounds (r.removeFromTop (64));
    r.removeFromTop (6);

    auto macros = r.removeFromTop (88);
    auto place = [&] (Knob& k, juce::Rectangle<int> cell)
    {
        k.l.setBounds (cell.removeFromTop (14));
        k.s.setBounds (cell);
    };
    const int mw = macros.getWidth() / 8;
    place (heat, macros.removeFromLeft (mw).reduced (2, 0));
    place (spread, macros.removeFromLeft (mw).reduced (2, 0));
    place (motionAmt, macros.removeFromLeft (mw).reduced (2, 0));
    place (globDrive, macros.removeFromLeft (mw).reduced (2, 0));
    place (mix, macros.removeFromLeft (mw).reduced (2, 0));
    place (out, macros.removeFromLeft (mw).reduced (2, 0));
    place (tone, macros.removeFromLeft (mw).reduced (2, 0));
    place (blend, macros.reduced (2, 0));

    r.removeFromTop (4);
    auto work = r.removeFromTop (300);
    focus.setBounds (work.removeFromLeft (520));
    work.removeFromLeft (8);
    auto lens = work;
    auto lensBar = lens.removeFromTop (24);
    routeLabel.setBounds (lensBar.removeFromLeft (36));
    routeBox.setBounds (lensBar.removeFromLeft (90));
    chmodeBox.setBounds (lensBar.removeFromLeft (80));
    stampBox.setBounds (lensBar.removeFromLeft (90));
    lensOn.setBounds (lensBar.removeFromLeft (36));
    lensMute.setBounds (lensBar.removeFromLeft (44));
    lensSolo.setBounds (lensBar.removeFromLeft (44));
    colorcomp.setBounds (lensBar.removeFromLeft (52));
    auto row1 = lens.removeFromTop (90);
    const int c1 = row1.getWidth() / 5;
    place (drive, row1.removeFromLeft (c1).reduced (2, 0));
    place (bias, row1.removeFromLeft (c1).reduced (2, 0));
    place (lensMix, row1.removeFromLeft (c1).reduced (2, 0));
    place (lensDyn, row1.removeFromLeft (c1).reduced (2, 0));
    place (lensLevel, row1.reduced (2, 0));
    auto row2 = lens.removeFromTop (90);
    const int c2 = row2.getWidth() / 5;
    place (lensPan, row2.removeFromLeft (c2).reduced (2, 0));
    place (lensFb, row2.removeFromLeft (c2).reduced (2, 0));
    place (lensFbHz, row2.removeFromLeft (c2).reduced (2, 0));
    place (filtHz, row2.removeFromLeft (c2).reduced (2, 0));
    place (filtQ, row2.reduced (2, 0));
    auto row3 = lens.removeFromTop (90);
    const int c3 = row3.getWidth() / 5;
    place (bass, row3.removeFromLeft (c3).reduced (2, 0));
    place (mid, row3.removeFromLeft (c3).reduced (2, 0));
    place (treble, row3.removeFromLeft (c3).reduced (2, 0));
    place (presence, row3.removeFromLeft (c3).reduced (2, 0));
    auto filtRow = row3;
    styleLabel.setBounds (filtRow.removeFromTop (14));
    styleBox.setBounds (filtRow.removeFromTop (22));
    filtBox.setBounds (filtRow);

    r.removeFromTop (4);
    auto ctx = r.removeFromTop (88);
    const int cw = ctx.getWidth() / 8;
    place (cavAmt, ctx.removeFromLeft (cw).reduced (2, 0));
    place (cavTime, ctx.removeFromLeft (cw).reduced (2, 0));
    place (cavNote, ctx.removeFromLeft (cw).reduced (2, 0));
    place (comp, ctx.removeFromLeft (cw).reduced (2, 0));
    place (xover1, ctx.removeFromLeft (cw).reduced (2, 0));
    place (xover2, ctx.removeFromLeft (cw).reduced (2, 0));
    place (xover3, ctx.removeFromLeft (cw).reduced (2, 0));
    auto cavCtl = ctx;
    fbModeBox.setBounds (cavCtl.removeFromTop (22));
    cavSafe.setBounds (cavCtl.removeFromTop (22));

    auto motionRow = r.removeFromTop (28);
    wireLabel.setBounds (motionRow.removeFromLeft (52));
    srcBox.setBounds (motionRow.removeFromLeft (80));
    destBox.setBounds (motionRow.removeFromLeft (120));
    wireOn.setBounds (motionRow.removeFromLeft (56));
    wireDepth.setBounds (motionRow.removeFromLeft (160));
    helpText.setBounds (r);
}
