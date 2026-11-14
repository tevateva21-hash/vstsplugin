#include "PluginEditor.h"
using namespace Eclipse;

// =============================================================== Grille de pas
StepGrid::StepGrid (EclipseProcessor& p, int t, Brush& b) : proc (p), track (t), brush (b) {}

void StepGrid::paint (juce::Graphics& g)
{
    const int steps = proc.getSteps (track), cw = TrackRow::cell, ph = proc.playStep[track];
    for (int s = 0; s < maxSteps; ++s)
    {
        auto r = juce::Rectangle<float> ((float) s * cw + 1.5f, 4.0f, (float) cw - 3.0f, (float) getHeight() - 8.0f);
        const bool on = s < steps;
        g.setColour (! on ? bg.brighter (0.02f) : ((s / 4) % 2 == 0 ? panelHi : panel.brighter (0.02f)));
        g.fillRoundedRectangle (r, 3.0f);
        const int v = proc.getVel (track, s);
        if (on && v > 0)
        {
            g.setColour (redDark.interpolatedWith (redHot, v / 127.0f));
            g.fillRoundedRectangle (r, 3.0f);
            const int fx = proc.getFx (track, s);
            if (fx > 0)
            {
                g.setColour (juce::Colours::white.withAlpha (0.9f)); g.setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));
                g.drawText (fxTag (fx), r.toNearestInt(), juce::Justification::centred);
            }
        }
        if (on && s == ph) { g.setColour (juce::Colours::white.withAlpha (0.75f)); g.drawRoundedRectangle (r, 3.0f, 1.5f); }
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

// =============================================================== FX lane
LaneGrid::LaneGrid (EclipseProcessor& p, Brush& b) : proc (p), brush (b) {}

void LaneGrid::paint (juce::Graphics& g)
{
    const int steps = proc.getLaneSteps(), cw = TrackRow::cell, ph = proc.lanePlay;
    for (int s = 0; s < maxSteps; ++s)
    {
        auto r = juce::Rectangle<float> ((float) s * cw + 1.5f, 4.0f, (float) cw - 3.0f, (float) getHeight() - 8.0f);
        const bool on = s < steps;
        g.setColour (! on ? bg.brighter (0.02f) : ((s / 4) % 2 == 0 ? panelHi : panel.brighter (0.02f)));
        g.fillRoundedRectangle (r, 3.0f);
        const int fx = proc.getLane (s);
        if (on && fx > 0)
        {
            g.setColour (red.darker (0.1f)); g.fillRoundedRectangle (r, 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.95f)); g.setFont (juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")));
            g.drawText (laneTag (fx), r.toNearestInt(), juce::Justification::centred);
        }
        if (on && s == ph) { g.setColour (juce::Colours::white.withAlpha (0.75f)); g.drawRoundedRectangle (r, 3.0f, 1.5f); }
    }
}
int LaneGrid::stepAt (juce::Point<int> p) const { const int s = p.x / TrackRow::cell; return (s >= 0 && s < proc.getLaneSteps()) ? s : -1; }
void LaneGrid::apply (const juce::MouseEvent& e, bool first)
{
    const int s = stepAt (e.getPosition());
    if (s < 0 || (s == lastStep && ! first)) return;
    lastStep = s;
    if (e.mods.isPopupMenu()) proc.setLane (s, (proc.getLane (s) + 1) % LaneCount);
    else
    {
        if (first) erasing = proc.getLane (s) == brush.laneFx.load();
        proc.setLane (s, erasing ? 0 : brush.laneFx.load());
    }
    repaint();
}
void LaneGrid::mouseDown (const juce::MouseEvent& e) { lastStep = -1; apply (e, true); }
void LaneGrid::mouseDrag (const juce::MouseEvent& e) { if (! e.mods.isPopupMenu()) apply (e, false); }

// =============================================================== Ligne de piste
TrackRow::TrackRow (EclipseProcessor& p, int t, Brush& b) : proc (p), track (t), grid (p, t, b)
{
    auto id = "t" + juce::String (t) + "_";
    padBtn.setButtonText ("P" + juce::String (t + 1));
    padBtn.setTooltip ("Cliquer pour ecouter le sample de la piste " + juce::String (t + 1));
    padBtn.onClick = [this] { proc.triggerPreview (track); };
    addAndMakeVisible (padBtn);

    mute.setClickingTogglesState (true);
    mute.setTooltip ("Muter la piste");
    addAndMakeVisible (mute);
    menu.onClick = [this] { showMenu(); }; addAndMakeVisible (menu);
    for (int i = 0; i < numSamples; ++i) sample.addItem (sampleNames()[i], i + 1);
    sample.addItem ("[ Mon sample ]", numSamples + 1);
    sample.onChange = [this] { refreshTooltip(); };
    load.onClick = [this] { chooseFile(); }; addAndMakeVisible (load);
    rate.addItemList ({ "1/8", "1/16", "1/16T", "1/32" }, 1);
    addAndMakeVisible (sample); addAndMakeVisible (rate);
    steps.setSliderStyle (juce::Slider::IncDecButtons); steps.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 26, 22);
    addAndMakeVisible (steps); addAndMakeVisible (grid);
    aSample = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id + "sample", sample);
    aRate = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id + "rate", rate);
    aMute = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id + "mute", mute);
    aSteps = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id + "steps", steps);
    const char* ids[6] = { "vol", "pan", "pitch", "len", "dly", "rev" };
    for (int i = 0; i < 6; ++i) { knobs[i] = std::make_unique<Knob> (p.apvts, id + ids[i], ids[i], false); addAndMakeVisible (*knobs[i]); }
    refreshTooltip();
}

void TrackRow::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (0.f, 2.f);
    g.setColour (panel.withAlpha (0.85f));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (line.withAlpha (0.7f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
}

void TrackRow::resized()
{
    auto r = getLocalBounds().reduced (6, 4);
    padBtn.setBounds (r.removeFromLeft (32).withSizeKeepingCentre (32, 28)); r.removeFromLeft (4);
    mute.setBounds (r.removeFromLeft (26).withSizeKeepingCentre (26, 26)); r.removeFromLeft (6);
    sample.setBounds (r.removeFromLeft (126).withSizeKeepingCentre (126, 26)); r.removeFromLeft (4);
    load.setBounds (r.removeFromLeft (44).withSizeKeepingCentre (44, 26)); r.removeFromLeft (6);
    rate.setBounds (r.removeFromLeft (60).withSizeKeepingCentre (60, 26)); r.removeFromLeft (4);
    steps.setBounds (r.removeFromLeft (68).withSizeKeepingCentre (68, 26)); r.removeFromLeft (8);
    grid.setBounds (r.removeFromLeft (cell * maxSteps)); r.removeFromLeft (8);
    for (auto& k : knobs) k->setBounds (r.removeFromLeft (40));
    r.removeFromLeft (4);
    menu.setBounds (r.removeFromLeft (28).withSizeKeepingCentre (28, 26));
}

bool TrackRow::isInterestedInFileDrag (const juce::StringArray& f)
{
    for (auto& n : f) if (juce::File (n).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg")) return true;
    return false;
}
void TrackRow::filesDropped (const juce::StringArray& f, int, int)
{
    if (f.size() > 0 && proc.loadUserSample (track, juce::File (f[0]))) refreshTooltip();
}
void TrackRow::refreshTooltip()
{
    auto n = proc.getUserName (track);
    if (n.isNotEmpty()) sample.changeItemText (numSamples + 1, "Sample: " + n);
    load.setTooltip (n.isEmpty() ? "Charger un sample (ou glisser un fichier sur la piste)" : "Sample : " + n);
}
void TrackRow::chooseFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Charger un sample sur la piste " + juce::String (track + 1),
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, [this] (const juce::FileChooser& fc)
    {
        auto f = fc.getResult();
        if (f.existsAsFile() && proc.loadUserSample (track, f)) refreshTooltip();
    });
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
EclipseEditor::EclipseEditor (EclipseProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      tabs ({ "1. SEQUENCEUR & SAMPLES", "2. EFFETS MAITRE & CLAVIER MIDI" }, [this] (int idx) { setTab (idx); }),
      laneGrid (p, brush),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (tabs);

    for (int t = 0; t < numTracks; ++t) { rows.push_back (std::make_unique<TrackRow> (p, t, brush)); addAndMakeVisible (*rows.back()); }

    addAndMakeVisible (laneGrid);
    laneSteps.setSliderStyle (juce::Slider::IncDecButtons); laneSteps.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 26, 22);
    addAndMakeVisible (laneSteps);
    aLaneSteps = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.apvts, "lane_steps", laneSteps);
    laneMenu.onClick = [this]
    {
        juce::PopupMenu m; m.addItem (1, "Vider la FX lane"); m.addItem (2, "Aleatoire 15%"); m.addItem (3, "Aleatoire 30%");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (laneMenu), [this] (int r)
        { if (r == 1) proc.clearLane(); else if (r == 2) proc.randomizeLane (0.15f); else if (r == 3) proc.randomizeLane (0.3f); laneGrid.repaint(); });
    };
    addAndMakeVisible (laneMenu);
    for (int f = 1; f < LaneCount; ++f) laneBrush.addItem (laneName (f), f);
    laneBrush.setSelectedId (1); laneBrush.onChange = [this] { brush.laneFx = laneBrush.getSelectedId(); };
    addAndMakeVisible (laneBrush);
    play.setClickingTogglesState (true); addAndMakeVisible (play);
    aPlay = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "run", play);
    bpm = std::make_unique<Knob> (p.apvts, "bpm", "BPM"); swing = std::make_unique<Knob> (p.apvts, "swing", "SWING");
    master = std::make_unique<Knob> (p.apvts, "master", "MASTER");
    addAndMakeVisible (*bpm); addAndMakeVisible (*swing); addAndMakeVisible (*master);

    int count; auto* pr = patternPresets (count);
    for (int i = 0; i < count; ++i) presets.addItem (pr[i].name, i + 1);
    presets.setSelectedId (1, juce::dontSendNotification);
    presets.onChange = [this] { proc.loadPreset (presets.getSelectedId() - 1); repaint(); };
    addAndMakeVisible (presets);
    clearBtn.onClick = [this] { proc.clearAll(); repaint(); };
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

    // Panneaux d'effets pour l'onglet 2
    auto makePanel = [this, &p] (const juce::String& title)
    {
        fxPanels.push_back (std::make_unique<Panel> (p.apvts, title));
        addChildComponent (*fxPanels.back());
        return fxPanels.back().get();
    };
    auto* dlyP = makePanel ("PSYCHEDELIC TAPE / BBD DELAY");
    dlyP->addCombo ("dlyTime", "TEMPS", 100);
    dlyP->addKnob ("dlyFb", "FEEDBACK", 74); dlyP->addKnob ("dlyTone", "TONE", 74);
    dlyP->addKnob ("dlyWob", "WOBBLE", 74); dlyP->addKnob ("dlyWobRate", "W.RATE", 74);
    dlyP->addKnob ("dlySpread", "PING-PONG", 74); dlyP->addKnob ("dlySmear", "SMEAR", 74);
    dlyP->addKnob ("dlyDrive", "DRIVE", 74); dlyP->addKnob ("dlyMix", "RETURN", 74);

    auto* fltP = makePanel ("FILTRE MAITRE + LFO");
    fltP->addCombo ("fType", "TYPE", 120);
    fltP->addKnob ("fCut", "CUTOFF", 80); fltP->addKnob ("fRes", "RESONANCE", 80);
    fltP->addKnob ("fLfo", "LFO PROF", 80); fltP->addKnob ("fRate", "LFO VIT", 80);

    auto* rvP = makePanel ("REVERB SPATIALE");
    rvP->addKnob ("rvSize", "TAILLE", 80); rvP->addKnob ("rvDamp", "DAMPING", 80); rvP->addKnob ("rvMix", "RETURN", 80);

    auto* phP = makePanel ("PHASER PSYCHEDELIQUE");
    phP->addKnob ("phRate", "VITESSE", 80); phP->addKnob ("phDepth", "PROFONDEUR", 80); phP->addKnob ("phMix", "MIX", 80);

    for (int f = 0; f < FxCount; ++f) brushFx.addItem (fxName (f), f + 1);
    brushFx.setSelectedId (1); brushFx.onChange = [this] { brush.fx = brushFx.getSelectedId() - 1; };
    addAndMakeVisible (brushFx);
    brushVel.setSliderStyle (juce::Slider::LinearHorizontal); brushVel.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    brushVel.setRange (10, 127, 1); brushVel.setValue (100); brushVel.onValueChange = [this] { brush.vel = (int) brushVel.getValue(); };
    addAndMakeVisible (brushVel);
    brushLabel.setText ("P1..P8: ecouter le sample  |  Clic/Glisser: poser pas  |  Clic droit: effet par pas  |  Molette: velocite  |  LOAD ou Glisser-Deposer: charger sample",
                        juce::dontSendNotification);
    brushLabel.setFont (juce::Font (juce::FontOptions (11.5f)));
    brushLabel.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (brushLabel);

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, redDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, red);
    keyboard.setAvailableRange (36, 84); keyboard.setKeyWidth (22.0f);
    addChildComponent (keyboard);

    setSize (1260, 720);
    setTab (0);
    startTimerHz (30);
}

EclipseEditor::~EclipseEditor() { setLookAndFeel (nullptr); }

void EclipseEditor::timerCallback()
{
    if (tabs.current() == 0)
        for (auto& r : rows) r->repaint();
    laneGrid.repaint();
}

void EclipseEditor::setTab (int idx)
{
    const bool seqTab = (idx == 0);
    for (auto& r : rows) r->setVisible (seqTab);
    brushFx.setVisible (seqTab);
    brushVel.setVisible (seqTab);
    laneBrush.setVisible (seqTab);
    brushLabel.setVisible (seqTab);
    for (auto& fp : fxPanels) fp->setVisible (! seqTab);
    keyboard.setVisible (! seqTab);
    resized();
    repaint();
}

void EclipseEditor::panelBox (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto rf = r.toFloat();
    // Ombre décalée franche style comic book
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (rf.translated (1.5f, 2.5f), 5.0f);

    // Fond noir d'encre épuré (pas de pavé anthracite massif)
    g.setColour (panel);
    g.fillRoundedRectangle (rf, 5.0f);

    auto head = rf.removeFromTop (24.0f);
    g.setColour (panelHi.withAlpha (0.6f));
    g.fillRoundedRectangle (head.getX() + 1.0f, head.getY() + 1.0f, head.getWidth() - 2.0f, head.getHeight(), 4.0f);
    g.setColour (red);
    g.fillRoundedRectangle (head.getX() + 8.0f, head.getY() + 5.0f, 3.5f, 14.0f, 1.5f);

    g.setColour (line);
    g.drawRoundedRectangle (rf.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (text);
    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    g.drawText (title, r.getX() + 18, r.getY() + 4, 340, 16, juce::Justification::centredLeft);
}

void EclipseEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (redHot); g.setFont (juce::Font (juce::FontOptions (28.0f).withStyle ("Bold")));
    g.drawText ("ECLIPSE", 18, 8, 200, 32, juce::Justification::left);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("PSYCHEDELIC SAMPLER & SEQUENCER", 20, 38, 250, 14, juce::Justification::left);
    g.setColour (line); g.drawHorizontalLine (64, 15.0f, (float) getWidth() - 15);

    if (tabs.current() == 0)
    {
        g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));
        const int y = 108, x0 = 15 + 6 + 32 + 4 + 26 + 6;
        g.drawText ("SAMPLE / LOAD (ou glisser)", x0, y, 174, 14, juce::Justification::left);
        g.drawText ("GRILLE", x0 + 180, y, 60, 14, juce::Justification::centred);
        g.drawText ("PAS", x0 + 244, y, 68, 14, juce::Justification::centred);
        const int gx = x0 + 320;
        g.drawText ("SEQUENCE (32 PAS)", gx, y, 220, 14, juce::Justification::left);
        const int kx = gx + TrackRow::cell * maxSteps + 8;
        const char* kn[6] = { "VOL", "PAN", "PITCH", "LEN", "DLY", "REV" };
        for (int i = 0; i < 6; ++i) g.drawText (kn[i], kx + i * 40, y, 40, 14, juce::Justification::centred);

        panelBox (g, pBrush, "PINCEAU DE SEQUENCE & OUTILS DE GRILLE");
        g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
        g.drawText ("EFFET PAR PAS :", pBrush.getX() + 16, pBrush.getY() + 34, 120, 24, juce::Justification::centredLeft);
        g.drawText ("VELOCITE :", pBrush.getX() + 310, pBrush.getY() + 34, 80, 24, juce::Justification::centredLeft);
        g.drawText ("EFFET FX LANE :", pBrush.getX() + 560, pBrush.getY() + 34, 120, 24, juce::Justification::centredLeft);
    }

    // Bandeau FX LANE (toujours visible)
    g.setColour (redHot); g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    g.drawText ("FX LANE MAITRE", 24, laneY, 130, laneH, juce::Justification::centredLeft);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (10.5f)));
    g.drawText ("T throw  F freeze  W swell  / \\ H filtres  R r repeat  < tape stop  B crush  G gate  M ring  D drive  X drop",
                900, laneY, 340, laneH, juce::Justification::centredLeft);
}

void EclipseEditor::resized()
{
    auto r = getLocalBounds().reduced (15, 0);
    play.setBounds (245, 16, 95, 32);
    bpm->setBounds (348, 2, 58, 58);
    swing->setBounds (410, 2, 58, 58);
    master->setBounds (472, 2, 58, 58);
    presets.setBounds (542, 16, 210, 30);
    clearBtn.setBounds (760, 16, 68, 30);
    exportBtn.setBounds (834, 16, 108, 30);
    keyLabel.setBounds (getWidth() - 220, 8, 100, 14);
    keyTrack.setBounds (getWidth() - 220, 24, 105, 26);

    tabs.setBounds (r.getX(), 70, r.getWidth(), 32);

    const int x0 = r.getX() + 6 + 32 + 4 + 26 + 6;
    if (tabs.current() == 0)
    {
        int y = 124;
        for (auto& row : rows) { row->setBounds (r.getX(), y, r.getWidth(), TrackRow::rowH); y += TrackRow::rowH + 3; }
        laneY = y + 4; laneH = 44;
        laneSteps.setBounds (x0 + 244, laneY + 9, 68, 26);
        laneGrid.setBounds (x0 + 320, laneY, TrackRow::cell * maxSteps, laneH);
        laneMenu.setBounds (x0 + 320 + TrackRow::cell * maxSteps + 8, laneY + 9, 32, 26);

        pBrush = { r.getX(), laneY + laneH + 8, r.getWidth(), 92 };
        brushFx.setBounds (pBrush.getX() + 130, pBrush.getY() + 34, 165, 26);
        brushVel.setBounds (pBrush.getX() + 385, pBrush.getY() + 34, 155, 26);
        laneBrush.setBounds (pBrush.getX() + 675, pBrush.getY() + 34, 180, 26);
        brushLabel.setBounds (pBrush.getX() + 16, pBrush.getY() + 64, pBrush.getWidth() - 32, 22);
    }
    else
    {
        laneY = 114; laneH = 48;
        laneSteps.setBounds (x0 + 244, laneY + 11, 68, 26);
        laneGrid.setBounds (x0 + 320, laneY, TrackRow::cell * maxSteps, laneH);
        laneMenu.setBounds (x0 + 320 + TrackRow::cell * maxSteps + 8, laneY + 11, 32, 26);

        const int py = laneY + laneH + 14;
        const int rowH = 165;
        layoutRow (r.getX(), py, { fxPanels[0].get() }, r.getWidth(), rowH);
        layoutRow (r.getX(), py + rowH + 12, { fxPanels[1].get(), fxPanels[2].get(), fxPanels[3].get() }, r.getWidth(), rowH);
        keyboard.setBounds (r.getX(), py + (rowH + 12) * 2 + 6, r.getWidth(), getHeight() - (py + (rowH + 12) * 2 + 18));
    }
}

