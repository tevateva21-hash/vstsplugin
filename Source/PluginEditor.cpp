#include "PluginEditor.h"
using namespace EclipseUI;
using namespace Eclipse;

// =============================================================== Look & feel (noir/rouge mat)
EclipseLookAndFeel::EclipseLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panelHi);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::arrowColourId, red);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, redDark);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, panelHi);
    setColour (juce::TextButton::buttonOnColourId, redDark);
    setColour (juce::TextButton::textColourOffId, textDim);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Label::textColourId, textDim);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, panelHi);
    setColour (juce::Slider::textBoxOutlineColourId, line);
    setColour (juce::Slider::trackColourId, red);
    setColour (juce::TooltipWindow::backgroundColourId, panel);
    setColour (juce::TooltipWindow::textColourId, text);
}

void EclipseLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float s, float e, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = std::min (r.getWidth(), r.getHeight());
    auto c = r.getCentre(); const float rad = d * 0.5f;
    const float ang = s + pos * (e - s);
    g.setColour (panelHi); g.fillEllipse (c.x - rad * 0.72f, c.y - rad * 0.72f, rad * 1.44f, rad * 1.44f);
    juce::Path track; track.addCentredArc (c.x, c.y, rad - 1.5f, rad - 1.5f, 0, s, e, true);
    g.setColour (line); g.strokePath (track, juce::PathStrokeType (2.5f));
    juce::Path val; val.addCentredArc (c.x, c.y, rad - 1.5f, rad - 1.5f, 0, s, ang, true);
    g.setColour (red); g.strokePath (val, juce::PathStrokeType (2.5f));
    g.setColour (redHot);
    g.drawLine (c.x + std::sin (ang) * rad * 0.25f, c.y - std::cos (ang) * rad * 0.25f,
                c.x + std::sin (ang) * rad * 0.68f, c.y - std::cos (ang) * rad * 0.68f, 2.0f);
}

void EclipseLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    g.setColour (panelHi); g.fillRoundedRectangle (0, 0, (float) w, (float) h, 2.0f);
    g.setColour (line); g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1, (float) h - 1, 2.0f, 1.0f);
    juce::Path a; const float cx = (float) w - 10, cy = h * 0.5f;
    a.addTriangle (cx - 3.5f, cy - 2, cx + 3.5f, cy - 2, cx, cy + 3);
    g.setColour (red); g.fillPath (a);
}

void EclipseLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (b.getToggleState() ? red.darker (0.2f) : (over ? panelHi.brighter (0.1f) : panelHi));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (b.getToggleState() ? red : line); g.drawRoundedRectangle (r, 2.0f, 1.0f);
}

void EclipseLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearHorizontal) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s); return; }
    const float cy = y + h * 0.5f;
    g.setColour (line); g.fillRect ((float) x, cy - 1.5f, (float) w, 3.0f);
    g.setColour (red); g.fillRect ((float) x, cy - 1.5f, pos - x, 3.0f);
    g.setColour (redHot); g.fillRect (pos - 3.0f, cy - 7.0f, 6.0f, 14.0f);
}

// =============================================================== Knob
Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name, bool withLabel) : hasLabel (withLabel)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification); label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (10.5f))); label.setInterceptsMouseClicks (false, false);
    if (hasLabel) addAndMakeVisible (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}
void Knob::resized()
{
    auto r = getLocalBounds();
    if (hasLabel) label.setBounds (r.removeFromBottom (13));
    slider.setBounds (r);
}

// =============================================================== Grille de pas
StepGrid::StepGrid (EclipseProcessor& p, int t, Brush& b) : proc (p), track (t), brush (b) {}

void StepGrid::paint (juce::Graphics& g)
{
    const int steps = proc.getSteps (track), cw = TrackRow::cell, ph = proc.playStep[track];
    for (int s = 0; s < maxSteps; ++s)
    {
        auto r = juce::Rectangle<float> ((float) s * cw + 1, 3.0f, (float) cw - 2, (float) getHeight() - 6);
        const bool on = s < steps;
        g.setColour (! on ? bg.brighter (0.02f) : ((s / 4) % 2 == 0 ? panelHi : panel.brighter (0.01f)));
        g.fillRoundedRectangle (r, 2.0f);
        const int v = proc.getVel (track, s);
        if (on && v > 0)
        {
            g.setColour (redDark.interpolatedWith (redHot, v / 127.0f));
            g.fillRoundedRectangle (r, 2.0f);
            const int fx = proc.getFx (track, s);
            if (fx > 0)
            {
                g.setColour (juce::Colours::white.withAlpha (0.85f)); g.setFont (juce::Font (juce::FontOptions (10.0f)));
                g.drawText (fxTag (fx), r.toNearestInt(), juce::Justification::centred);
            }
        }
        if (on && s == ph) { g.setColour (juce::Colours::white.withAlpha (0.55f)); g.drawRoundedRectangle (r, 2.0f, 1.2f); }
    }
}

int StepGrid::stepAt (juce::Point<int> p) const
{
    const int s = p.x / TrackRow::cell;
    return (s >= 0 && s < proc.getSteps (track)) ? s : -1;
}

void StepGrid::apply (const juce::MouseEvent& e, bool first)
{
    const int s = stepAt (e.getPosition());
    if (s < 0 || (s == lastStep && ! first)) return;
    lastStep = s;
    if (e.mods.isPopupMenu())
    {
        const int v = proc.getVel (track, s) == 0 ? brush.vel.load() : proc.getVel (track, s);
        proc.setStep (track, s, v, (proc.getFx (track, s) + 1) % FxCount);
    }
    else
    {
        if (first) erasing = proc.getVel (track, s) > 0 && proc.getFx (track, s) == brush.fx.load();
        if (erasing) proc.setStep (track, s, 0, 0); else proc.setStep (track, s, brush.vel, brush.fx);
    }
    repaint();
}
void StepGrid::mouseDown (const juce::MouseEvent& e) { lastStep = -1; apply (e, true); }
void StepGrid::mouseDrag (const juce::MouseEvent& e) { if (! e.mods.isPopupMenu()) apply (e, false); }
void StepGrid::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const int s = stepAt (e.getPosition());
    if (s < 0 || proc.getVel (track, s) == 0) return;
    proc.setStep (track, s, juce::jlimit (10, 127, proc.getVel (track, s) + (w.deltaY > 0 ? 8 : -8)), proc.getFx (track, s));
    repaint();
}

// =============================================================== Ligne de piste
TrackRow::TrackRow (EclipseProcessor& p, int t, Brush& b) : proc (p), track (t), grid (p, t, b)
{
    auto id = "t" + juce::String (t) + "_";
    num.setText (juce::String (t + 1), juce::dontSendNotification); num.setJustificationType (juce::Justification::centred);
    num.setColour (juce::Label::textColourId, red);
    addAndMakeVisible (num);
    mute.setClickingTogglesState (true); addAndMakeVisible (mute);
    menu.onClick = [this] { showMenu(); }; addAndMakeVisible (menu);
    for (int i = 0; i < numSamples; ++i) sample.addItem (sampleNames()[i], i + 1);
    rate.addItemList ({ "1/8", "1/16", "1/16T", "1/32" }, 1);
    addAndMakeVisible (sample); addAndMakeVisible (rate);
    steps.setSliderStyle (juce::Slider::IncDecButtons); steps.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 24, 20);
    addAndMakeVisible (steps); addAndMakeVisible (grid);
    aSample = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id + "sample", sample);
    aRate = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id + "rate", rate);
    aMute = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id + "mute", mute);
    aSteps = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id + "steps", steps);
    const char* ids[6] = { "vol", "pan", "pitch", "len", "dly", "rev" };
    for (int i = 0; i < 6; ++i) { knobs[i] = std::make_unique<Knob> (p.apvts, id + ids[i], ids[i], false); addAndMakeVisible (*knobs[i]); }
}

void TrackRow::resized()
{
    auto r = getLocalBounds().reduced (0, 3);
    num.setBounds (r.removeFromLeft (20)); r.removeFromLeft (3);
    mute.setBounds (r.removeFromLeft (24).withSizeKeepingCentre (24, 24)); r.removeFromLeft (4);
    sample.setBounds (r.removeFromLeft (140).withSizeKeepingCentre (140, 24)); r.removeFromLeft (4);
    rate.setBounds (r.removeFromLeft (56).withSizeKeepingCentre (56, 24)); r.removeFromLeft (4);
    steps.setBounds (r.removeFromLeft (70).withSizeKeepingCentre (70, 24)); r.removeFromLeft (6);
    grid.setBounds (r.removeFromLeft (cell * maxSteps)); r.removeFromLeft (6);
    for (auto& k : knobs) k->setBounds (r.removeFromLeft (36));
    r.removeFromLeft (4);
    menu.setBounds (r.removeFromLeft (28).withSizeKeepingCentre (28, 24));
}

void TrackRow::showMenu()
{
    juce::PopupMenu m, eu, rd;
    for (int h : { 3, 4, 5, 7, 9, 11 }) eu.addItem (100 + h, juce::String (h) + " coups (Euclidien)");
    for (int d : { 25, 50, 75 }) rd.addItem (200 + d, juce::String (d) + "% densité");
    m.addSubMenu ("Euclidien", eu); m.addSubMenu ("Aléatoire", rd);
    m.addItem (300, "Décaler  <"); m.addItem (301, "Décaler  >"); m.addItem (302, "Inverser le pattern");
    m.addSeparator(); m.addItem (303, "Vider la piste");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menu), [this] (int r)
    {
        if (r >= 100 && r < 200) proc.euclid (track, r - 100, 0);
        else if (r >= 200 && r < 300) proc.randomizeTrack (track, (r - 200) / 100.0f);
        else if (r == 300) proc.shiftTrack (track, -1);
        else if (r == 301) proc.shiftTrack (track, 1);
        else if (r == 302) proc.reverseTrack (track);
        else if (r == 303) proc.clearTrack (track);
        grid.repaint();
    });
}

// =============================================================== Éditeur
EclipseEditor::EclipseEditor (EclipseProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    setSize (1120, 700);
    for (int t = 0; t < numTracks; ++t) { rows.push_back (std::make_unique<TrackRow> (p, t, brush)); addAndMakeVisible (*rows.back()); }

    play.setClickingTogglesState (true); addAndMakeVisible (play);
    aPlay = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "run", play);
    bpm = std::make_unique<Knob> (p.apvts, "bpm", "BPM"); swing = std::make_unique<Knob> (p.apvts, "swing", "SWING");
    master = std::make_unique<Knob> (p.apvts, "master", "MASTER");
    addAndMakeVisible (*bpm); addAndMakeVisible (*swing); addAndMakeVisible (*master);

    int count; auto* pr = patternPresets (count);
    for (int i = 0; i < count; ++i) presets.addItem (pr[i].name, i + 1);
    presets.setTextWhenNothingSelected ("Patterns MIDI proposés...");
    presets.onChange = [this] { proc.loadPreset (presets.getSelectedId() - 1); };
    addAndMakeVisible (presets);
    clearBtn.onClick = [this] { proc.clearAll(); };
    addAndMakeVisible (clearBtn);
    exportBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Exporter le pattern en MIDI", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("eclipse_pattern.mid"), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) proc.exportMidi (fc.getResult()); });
    };
    addAndMakeVisible (exportBtn);
    for (int i = 1; i <= 8; ++i) keyTrack.addItem ("Piste " + juce::String (i), i);
    addAndMakeVisible (keyTrack);
    aKey = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "keyTrack", keyTrack);
    keyLabel.setText ("CLAVIER MIDI >", juce::dontSendNotification); addAndMakeVisible (keyLabel);

    auto K = [this, &p] (const char* id, const char* n) { fxKnobs.push_back (std::make_unique<Knob> (p.apvts, id, n)); addAndMakeVisible (*fxKnobs.back()); };
    delayTime.addItemList ({ "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2" }, 1);
    addAndMakeVisible (delayTime);
    aDly = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "dlyTime", delayTime);
    K ("dlyFb", "FEEDBACK"); K ("dlyTone", "TONE"); K ("dlyWob", "WOBBLE"); K ("dlyWobRate", "W.RATE");
    K ("dlySpread", "PINGPONG"); K ("dlySmear", "SMEAR"); K ("dlyDrive", "DRIVE"); K ("dlyMix", "RETURN");
    K ("rvSize", "SIZE"); K ("rvDamp", "DAMP"); K ("rvMix", "RETURN");
    K ("phRate", "RATE"); K ("phDepth", "DEPTH"); K ("phMix", "MIX");
    filterType.addItemList ({ "Passe-bas", "Passe-haut", "Passe-bande" }, 1); addAndMakeVisible (filterType);
    aFType = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "fType", filterType);
    K ("fCut", "CUTOFF"); K ("fRes", "RESO"); K ("fLfo", "LFO AMT"); K ("fRate", "LFO RATE");

    for (int f = 0; f < FxCount; ++f) brushFx.addItem (fxName (f), f + 1);
    brushFx.setSelectedId (1); brushFx.onChange = [this] { brush.fx = brushFx.getSelectedId() - 1; };
    addAndMakeVisible (brushFx);
    brushVel.setSliderStyle (juce::Slider::LinearHorizontal); brushVel.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    brushVel.setRange (10, 127, 1); brushVel.setValue (100); brushVel.onValueChange = [this] { brush.vel = (int) brushVel.getValue(); };
    addAndMakeVisible (brushVel);
    brushLabel.setText ("PINCEAU (effet + vélocité)  -  clic: poser/effacer (glisser = peindre)  |  clic droit: changer l'effet du pas  |  molette: vélocité  |  bouton '...' de piste: euclidien, aléatoire, décalage",
                        juce::dontSendNotification);
    brushLabel.setFont (juce::Font (juce::FontOptions (10.5f))); addAndMakeVisible (brushLabel);
    startTimerHz (30);
}

EclipseEditor::~EclipseEditor() { setLookAndFeel (nullptr); }

void EclipseEditor::panelBox (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    g.setColour (panel); g.fillRoundedRectangle (r.toFloat(), 3.0f);
    g.setColour (line); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 3.0f, 1.0f);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")));
    g.drawText (title, r.getX() + 10, r.getY() + 5, 200, 14, juce::Justification::left);
}

void EclipseEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (30.0f).withStyle ("Bold")));
    g.drawText ("ECLIPSE", 18, 8, 220, 34, juce::Justification::left);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("PSYCHEDELIC SAMPLER", 20, 40, 220, 14, juce::Justification::left);
    g.setColour (line); g.drawHorizontalLine (68, 15.0f, (float) getWidth() - 15);

    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (10.0f)));
    const int y = 72, x0 = 15 + 20 + 3 + 24 + 4;
    g.drawText ("SAMPLE", x0, y, 140, 14, juce::Justification::left);
    g.drawText ("GRILLE", x0 + 144, y, 56, 14, juce::Justification::centred);
    g.drawText ("PAS", x0 + 204, y, 70, 14, juce::Justification::centred);
    const int gx = x0 + 144 + 4 + 56 + 4 + 70 + 6 - 4;
    g.drawText ("SEQUENCE", gx, y, 200, 14, juce::Justification::left);
    const int kx = gx + TrackRow::cell * maxSteps + 6; const char* kn[6] = { "VOL", "PAN", "PITCH", "LEN", "DLY", "REV" };
    for (int i = 0; i < 6; ++i) g.drawText (kn[i], kx + i * 36, y, 36, 14, juce::Justification::centred);

    panelBox (g, pDelay, "PSYCHEDELIC DELAY / ECHO"); panelBox (g, pRev, "REVERB");
    panelBox (g, pPhaser, "PHASER"); panelBox (g, pFilter, "FILTRE MAITRE + LFO"); panelBox (g, pBrush, "OUTILS DE GRILLE");
}

void EclipseEditor::resized()
{
    auto r = getLocalBounds().reduced (15, 0);
    auto head = juce::Rectangle<int> (r.getX(), 6, r.getWidth(), 58);
    play.setBounds (head.getX() + 260, 20, 70, 28);
    bpm->setBounds (head.getX() + 345, 4, 56, 62); swing->setBounds (head.getX() + 405, 4, 56, 62); master->setBounds (head.getX() + 465, 4, 56, 62);
    presets.setBounds (head.getX() + 545, 20, 200, 28);
    clearBtn.setBounds (head.getX() + 755, 20, 60, 28);
    exportBtn.setBounds (head.getX() + 820, 20, 100, 28);
    keyLabel.setBounds (head.getRight() - 120, 6, 110, 14);
    keyTrack.setBounds (head.getRight() - 120, 24, 100, 24);

    int y = 88;
    for (auto& row : rows) { row->setBounds (r.getX(), y, r.getWidth(), TrackRow::rowH); y += TrackRow::rowH; }

    const int py = 446;
    pDelay = { r.getX(), py, 640, 118 }; pRev = { r.getX() + 650, py, 195, 118 }; pPhaser = { r.getX() + 855, py, 195, 118 };
    pFilter = { r.getX(), py + 126, 430, 118 }; pBrush = { r.getX() + 440, py + 126, 610, 118 };

    auto place = [] (juce::Component& c, juce::Rectangle<int> panel, int idx) { c.setBounds (panel.getX() + 8 + idx * 58, panel.getY() + 26, 56, 84); };
    delayTime.setBounds (pDelay.getRight() - 90, pDelay.getY() + 4, 80, 20);
    for (int i = 0; i < 8; ++i) place (*fxKnobs[(size_t) i], pDelay, i);
    for (int i = 0; i < 3; ++i) place (*fxKnobs[(size_t) (8 + i)], pRev, i);
    for (int i = 0; i < 3; ++i) place (*fxKnobs[(size_t) (11 + i)], pPhaser, i);
    filterType.setBounds (pFilter.getRight() - 110, pFilter.getY() + 4, 100, 20);
    for (int i = 0; i < 4; ++i) place (*fxKnobs[(size_t) (14 + i)], pFilter, i);

    brushFx.setBounds (pBrush.getX() + 12, pBrush.getY() + 32, 170, 26);
    brushVel.setBounds (pBrush.getX() + 200, pBrush.getY() + 32, 200, 26);
    brushLabel.setBounds (pBrush.getX() + 12, pBrush.getY() + 66, pBrush.getWidth() - 24, 40);
}
