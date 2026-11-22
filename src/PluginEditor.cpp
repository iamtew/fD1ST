#include "PluginEditor.h"
#include "presets/Factory.h"

namespace
{
constexpr int kEditorW = 1100;
constexpr int kEditorH = 720;
constexpr int kPad = 10;
constexpr int kTitleH = 64;
constexpr int kLogoH = 56;

// Outer field is opaque black. Flood from the edges so the D sits on burgundy.
// Halftone dots inside the D stay — they are not edge-connected.
juce::Image punchEdgeBlack (juce::Image src)
{
    if (! src.isValid())
        return src;
    src = src.createCopy();
    if (src.getFormat() != juce::Image::ARGB)
        src = src.convertedToFormat (juce::Image::ARGB);
    juce::Image::BitmapData d (src, juce::Image::BitmapData::readWrite);
    const int w = src.getWidth(), h = src.getHeight();
    juce::HeapBlock<char> seen ((size_t) w * (size_t) h, true);
    juce::Array<juce::Point<int>> q;
    auto nearBlack = [&] (int x, int y)
    {
        auto c = d.getPixelColour (x, y);
        return c.getAlpha() > 8 && (c.getRed() + c.getGreen() + c.getBlue()) < 24;
    };
    auto push = [&] (int x, int y)
    {
        if (x < 0 || y < 0 || x >= w || y >= h)
            return;
        const int i = y * w + x;
        if (seen[(size_t) i])
            return;
        if (! nearBlack (x, y))
            return;
        seen[(size_t) i] = 1;
        q.add ({ x, y });
    };
    for (int x = 0; x < w; ++x)
    {
        push (x, 0);
        push (x, h - 1);
    }
    for (int y = 0; y < h; ++y)
    {
        push (0, y);
        push (w - 1, y);
    }
    for (int n = 0; n < q.size(); ++n)
    {
        const auto p = q.getUnchecked (n);
        d.setPixelColour (p.x, p.y, juce::Colour());
        push (p.x + 1, p.y);
        push (p.x - 1, p.y);
        push (p.x, p.y + 1);
        push (p.x, p.y - 1);
    }
    return src;
}

juce::Image cropOpaque (juce::Image src)
{
    if (! src.isValid())
        return src;
    juce::Image::BitmapData d (src, juce::Image::BitmapData::readOnly);
    const int w = src.getWidth(), h = src.getHeight();
    int x0 = w, y0 = h, x1 = 0, y1 = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (d.getPixelColour (x, y).getAlpha() > 8)
            {
                x0 = juce::jmin (x0, x);
                y0 = juce::jmin (y0, y);
                x1 = juce::jmax (x1, x);
                y1 = juce::jmax (y1, y);
            }
    if (x1 <= x0)
        return src;
    return src.getClippedImage ({ x0, y0, x1 - x0 + 1, y1 - y0 + 1 }).createCopy();
}

juce::Rectangle<int> fitH (juce::Rectangle<int> cell, int h)
{
    return cell.withSizeKeepingCentre (cell.getWidth(), h);
}
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

void fD1STAudioProcessorEditor::hideKnob (Knob& k)
{
    k.s.setVisible (false);
    k.l.setVisible (false);
    k.s.setBounds ({});
    k.l.setBounds ({});
}

void fD1STAudioProcessorEditor::placeKnob (Knob& k, juce::Rectangle<int> cell)
{
    k.s.setVisible (true);
    k.l.setVisible (true);
    k.l.setBounds (cell.removeFromTop (14));
    k.s.setBounds (cell);
}

void fD1STAudioProcessorEditor::hideStageControls()
{
    for (auto* k : { &mix, &out, &globDrive, &drive, &bias, &blend, &tone, &cavTime, &cavAmt, &cavNote, &comp,
                     &lensMix, &lensDyn, &lensLevel, &lensPan, &lensFb, &lensFbHz, &filtHz, &filtQ, &bass, &mid,
                     &treble, &presence, &xover1, &xover2, &xover3 })
        hideKnob (*k);
    for (auto* c : { &styleBox, &chmodeBox, &stampBox, &fbModeBox, &filtBox })
    {
        c->setVisible (false);
        c->setBounds ({});
    }
    for (auto* b : { &colorcomp, &cavSafe, &lensOn, &lensMute, &lensSolo })
    {
        b->setVisible (false);
        b->setBounds ({});
    }
    styleLabel.setVisible (false);
    styleLabel.setBounds ({});
    routeLabel.setVisible (false);
    routeLabel.setBounds ({});
    stageHint.setVisible (false);
    stageHint.setBounds ({});
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
    panelLnF.setColour (juce::PopupMenu::backgroundColourId, kWell);
    panelLnF.setColour (juce::PopupMenu::textColourId, kSage);
    panelLnF.setColour (juce::PopupMenu::headerTextColourId, kEmerald);
    panelLnF.setColour (juce::PopupMenu::highlightedBackgroundColourId, kEmerald);
    panelLnF.setColour (juce::PopupMenu::highlightedTextColourId, kWhite);

    logo = punchEdgeBlack (juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize));
    logo = cropOpaque (logo);
    if (logo.isValid() && logo.getHeight() > 0)
    {
        const int dh = kLogoH * 2;
        const int dw = juce::jmax (1, dh * logo.getWidth() / logo.getHeight());
        logo = logo.rescaled (dw, dh, juce::Graphics::highResamplingQuality);
        logoDrawW = dw / 2;
    }

    bindKnob (mix, ParamID::glob_mix, "Mix");
    bindKnob (out, ParamID::glob_out_db, "Output");
    bindKnob (globDrive, ParamID::glob_drive_db, "Gain");
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
    for (int c = 0; c < fd1st::kFactoryCatCount; ++c)
    {
        const auto& cat = fd1st::kFactoryCats[c];
        programBox.addSectionHeading (cat.name);
        for (int i = cat.begin; i < cat.end; ++i)
            programBox.addItem (fd1st::factoryName (i), i + 1);
    }
    programBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    programBox.setTooltip ("Preset");
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
    routeLabel.setText ("Route", juce::dontSendNotification);
    progLabel.setText ("Preset", juce::dontSendNotification);
    wireLabel.setText ("Wiring", juce::dontSendNotification);
    stageHint.setJustificationType (juce::Justification::centred);
    stageHint.setColour (juce::Label::textColourId, kSage);
    content.addAndMakeVisible (stageHint);
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

    helpText.setText ("DIST: click Path to Focus a stage. W wet, Shift+D dry, E delta, C caustic.",
                      juce::dontSendNotification);
    helpText.setColour (juce::Label::textColourId, kSage);
    helpText.setVisible (false);
    content.addAndMakeVisible (helpText);

    path.onFocus = [this] (int s)
    {
        focusStage = s;
        if (s >= 0)
            rebindLens();
        layoutContent();
    };
    content.addAndMakeVisible (path);
    content.addAndMakeVisible (focus);
    content.addAndMakeVisible (context);
    rebindLens();
    routeBox.onChange = [this]
    {
        layoutContent();
        path.repaint();
    };

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
    layoutContent();
}

void fD1STAudioProcessorEditor::doRedo()
{
    if (undoPos + 1 >= undoStack.size())
        return;
    ++undoPos;
    auto& mb = undoStack.getReference (undoPos);
    proc.setStateInformation (mb.getData(), (int) mb.getSize());
    rebindLens();
    layoutContent();
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
        const int lw = logo.getWidth() / 2;
        const int lh = logo.getHeight() / 2;
        auto logoR = title.removeFromLeft (lw).withSizeKeepingCentre (lw, lh);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
        title.removeFromLeft (6);
    }
    g.setColour (kSage);
    g.setFont (boldFont (18.f));
    g.drawText ("fD1ST", title.removeFromLeft (56), juce::Justification::centredLeft, true);
}

void fD1STAudioProcessorEditor::layoutContent()
{
    auto r = content.getLocalBounds().reduced (kPad);
    auto bar = r.removeFromTop (kTitleH);
    const int logoW = logoDrawW > 0 ? logoDrawW : kLogoH;
    bar.removeFromLeft (logoW + 8 + 56);

    fullBtn.setBounds (fitH (bar.removeFromRight (40), 24));
    helpBtn.setBounds (fitH (bar.removeFromRight (22), 24));
    midiLearn.setBounds (fitH (bar.removeFromRight (40), 24));
    bypass.setBounds (fitH (bar.removeFromRight (52), 24));
    linphase.setBounds (fitH (bar.removeFromRight (32), 24));
    auto osArea = bar.removeFromRight (80);
    osLabel.setBounds (fitH (osArea.removeFromLeft (20), 18));
    osBox.setBounds (fitH (osArea, 24));
    listenC.setBounds (fitH (bar.removeFromRight (26), 24));
    listenE.setBounds (fitH (bar.removeFromRight (26), 24));
    listenD.setBounds (fitH (bar.removeFromRight (34), 24));
    listenW.setBounds (fitH (bar.removeFromRight (26), 24));

    progLabel.setVisible (false);
    progLabel.setBounds ({});
    programBox.setBounds (fitH (bar.removeFromLeft (110), 24));
    undoBtn.setBounds (fitH (bar.removeFromLeft (40), 24));
    redoBtn.setBounds (fitH (bar.removeFromLeft (40), 24));
    abA.setBounds (fitH (bar.removeFromLeft (22), 24));
    abB.setBounds (fitH (bar.removeFromLeft (22), 24));
    abMorph.l.setBounds (fitH (bar.removeFromLeft (22), 18));
    abMorph.s.setBounds (fitH (bar.removeFromLeft (56), 18));

    auto placeMacro = [&] (Knob& k)
    {
        placeKnob (k, bar.removeFromLeft (54).reduced (1, 0));
    };
    placeMacro (heat);
    placeMacro (spread);
    placeMacro (motionAmt);
    gpuBanner.setBounds (bar);

    r.removeFromTop (6);
    path.setBounds (r.removeFromTop (56));
    routeBox.setVisible (true);
    routeBox.setBounds (path.getX() + 8, path.getY() + 17, 80, 22);

    hideStageControls();
    const int route = (int) proc.apvts.getRawParameterValue (ParamID::glob_route)->load();
    const int bands = juce::jlimit (1, 6, (int) proc.apvts.getRawParameterValue (ParamID::glob_bands)->load());
    int maxLens = 0;
    if (route == 3)
        maxLens = bands - 1;
    else if (route == 1 || route == 2 || route == 4 || route == 5)
        maxLens = 1;
    if (focusStage > maxLens)
    {
        focusStage = 0;
        path.selected = 0;
        proc.focusLens.store (0);
        rebindLens();
    }

    r.removeFromTop (6);
    auto work = r.removeFromTop (340);
    focus.setBounds (work.removeFromLeft (520));
    work.removeFromLeft (8);
    auto stage = work;

    if (focusStage == PathView::In || focusStage == PathView::Drive)
    {
        auto row = stage.removeFromTop (110);
        const int cw = row.getWidth() / 3;
        placeKnob (globDrive, row.removeFromLeft (cw).reduced (4, 0));
        placeKnob (tone, row.removeFromLeft (cw).reduced (4, 0));
        auto ctl = row.reduced (8, 16);
        colorcomp.setVisible (true);
        colorcomp.setBounds (ctl.removeFromTop (24));
        chmodeBox.setVisible (true);
        chmodeBox.setBounds (ctl.removeFromTop (24));
    }
    else if (focusStage >= 0)
    {
        auto lensBar = stage.removeFromTop (24);
        stampBox.setVisible (true);
        stampBox.setBounds (lensBar.removeFromLeft (90));
        lensOn.setVisible (true);
        lensOn.setBounds (lensBar.removeFromLeft (36));
        lensMute.setVisible (true);
        lensMute.setBounds (lensBar.removeFromLeft (44));
        lensSolo.setVisible (true);
        lensSolo.setBounds (lensBar.removeFromLeft (44));
        styleLabel.setVisible (true);
        styleLabel.setBounds (lensBar.removeFromLeft (40));
        styleBox.setVisible (true);
        styleBox.setBounds (lensBar.removeFromLeft (90));
        filtBox.setVisible (true);
        filtBox.setBounds (lensBar.removeFromLeft (90));
        auto row1 = stage.removeFromTop (100);
        const int c1 = row1.getWidth() / 4;
        placeKnob (drive, row1.removeFromLeft (c1).reduced (2, 0));
        placeKnob (bias, row1.removeFromLeft (c1).reduced (2, 0));
        placeKnob (lensMix, row1.removeFromLeft (c1).reduced (2, 0));
        placeKnob (lensLevel, row1.reduced (2, 0));
        auto row2 = stage.removeFromTop (100);
        const int c2 = row2.getWidth() / 6;
        placeKnob (filtHz, row2.removeFromLeft (c2).reduced (2, 0));
        placeKnob (filtQ, row2.removeFromLeft (c2).reduced (2, 0));
        placeKnob (bass, row2.removeFromLeft (c2).reduced (2, 0));
        placeKnob (mid, row2.removeFromLeft (c2).reduced (2, 0));
        placeKnob (treble, row2.removeFromLeft (c2).reduced (2, 0));
        placeKnob (presence, row2.reduced (2, 0));
        auto row3 = stage.removeFromTop (100);
        const int c3 = row3.getWidth() / 4;
        placeKnob (lensDyn, row3.removeFromLeft (c3).reduced (2, 0));
        placeKnob (lensPan, row3.removeFromLeft (c3).reduced (2, 0));
        placeKnob (lensFb, row3.removeFromLeft (c3).reduced (2, 0));
        placeKnob (lensFbHz, row3.reduced (2, 0));
    }
    else if (focusStage == PathView::Cavity)
    {
        auto row = stage.removeFromTop (110);
        const int cw = row.getWidth() / 4;
        placeKnob (cavAmt, row.removeFromLeft (cw).reduced (2, 0));
        placeKnob (cavTime, row.removeFromLeft (cw).reduced (2, 0));
        placeKnob (cavNote, row.removeFromLeft (cw).reduced (2, 0));
        auto ctl = row.reduced (8, 16);
        fbModeBox.setVisible (true);
        fbModeBox.setBounds (ctl.removeFromTop (24));
        cavSafe.setVisible (true);
        cavSafe.setBounds (ctl.removeFromTop (24));
    }
    else if (focusStage == PathView::Comp)
    {
        placeKnob (comp, stage.removeFromTop (140).withSizeKeepingCentre (120, 140));
    }
    else if (focusStage == PathView::Out)
    {
        auto row = stage.removeFromTop (110);
        const int cw = row.getWidth() / 3;
        placeKnob (mix, row.removeFromLeft (cw).reduced (4, 0));
        placeKnob (out, row.removeFromLeft (cw).reduced (4, 0));
        placeKnob (blend, row.reduced (4, 0));
    }
    else
    {
        stageHint.setVisible (true);
        stageHint.setText ("Hard clip at the rail.", juce::dontSendNotification);
        stageHint.setBounds (stage);
    }

    r.removeFromTop (4);
    auto ctx = r.removeFromTop (100);
    if (route == 3)
    {
        auto xrow = ctx.removeFromRight (240);
        const int xw = xrow.getWidth() / 3;
        placeKnob (xover1, xrow.removeFromLeft (xw).reduced (2, 0));
        placeKnob (xover2, xrow.removeFromLeft (xw).reduced (2, 0));
        placeKnob (xover3, xrow.reduced (2, 0));
    }
    context.setBounds (ctx);

    auto motionRow = r.removeFromTop (28);
    wireLabel.setBounds (motionRow.removeFromLeft (52));
    srcBox.setBounds (motionRow.removeFromLeft (80));
    destBox.setBounds (motionRow.removeFromLeft (120));
    wireOn.setBounds (motionRow.removeFromLeft (56));
    wireDepth.setBounds (motionRow.removeFromLeft (160));
    helpText.setBounds (r);
}
