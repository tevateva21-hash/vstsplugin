#include "KinoEditor.h"

// =============================================================== Drag .MID Pill
void DragMidiPill::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (KinoColors::ink);
    g.fillRoundedRectangle (r.translated (1.5f, 2.0f), 5.0f); // petite ombre BD nette
    g.setColour (isMouseOver() ? KinoColors::red : KinoColors::card);
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (KinoColors::ink);
    g.drawRoundedRectangle (r, 5.0f, 1.5f);

    g.setColour (isMouseOver() ? juce::Colours::white : KinoColors::text);
    g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("DRAG .MID -> DAW", r.toNearestInt(), juce::Justification::centred);
}

void DragMidiPill::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging && e.getDistanceFromDragStart() > 4)
    {
        dragging = true;
        auto f = proc.createTempMidiFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
        dragging = false;
    }
}

// =============================================================== PianoRollGrid
PianoRollGrid::PianoRollGrid (KinoProcessor& p) : proc (p) {}

void PianoRollGrid::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (KinoColors::card);
    g.fillRoundedRectangle (r, 4.0f);

    const int nSteps = juce::jlimit (4, KinoProcessor::maxSteps, (int) proc.P ("steps"));
    const int playHead = proc.currentStep.load();
    const float gridW = r.getWidth() - (float) labelW;
    const float cellW = gridW / (float) nSteps;
    const float cellH = r.getHeight() / (float) numRows;

    // Colonne labels de notes sur la gauche
    g.setFont (juce::Font (juce::FontOptions (9.5f).withStyle ("Bold")));
    for (int row = 0; row < numRows; ++row)
    {
        const int deg = maxDeg - row;
        const float y = row * cellH;
        const bool isRoot = (deg == 0 || deg == 7);

        // Bande alternee ultra-legere
        if (isRoot)
        {
            g.setColour (KinoColors::redSoft.withAlpha (0.45f));
            g.fillRect ((float) labelW, y, gridW, cellH);
        }
        else if (row % 2 == 1)
        {
            g.setColour (KinoColors::bg.withAlpha (0.65f));
            g.fillRect ((float) labelW, y, gridW, cellH);
        }

        g.setColour (isRoot ? KinoColors::red : KinoColors::textDim);
        g.drawText (proc.noteNameForDegree (deg), 2, (int) y, labelW - 6, (int) cellH, juce::Justification::centredRight);

        g.setColour (KinoColors::border.withAlpha (0.7f));
        g.drawHorizontalLine ((int) y, (float) labelW, r.getWidth());
    }

    // Colonnes des pas + playhead + notes actives
    for (int s = 0; s < nSteps; ++s)
    {
        const float x = (float) labelW + s * cellW;
        if (s == playHead)
        {
            g.setColour (KinoColors::red.withAlpha (0.10f));
            g.fillRect (x, 0.0f, cellW, r.getHeight());
        }

        g.setColour ((s % 4 == 0) ? KinoColors::textDim.withAlpha (0.35f) : KinoColors::border.withAlpha (0.6f));
        g.drawVerticalLine ((int) x, 0.0f, r.getHeight());

        const auto st = proc.getStep (s);
        if (st.active)
        {
            const int clampedDeg = juce::jlimit (minDeg, maxDeg, st.degree);
            const int row = maxDeg - clampedDeg;
            auto nr = juce::Rectangle<float> (x + 2.0f, row * cellH + 1.5f, std::max (4.0f, cellW - 4.0f), std::max (4.0f, cellH - 3.0f));

            // Note active en Rouge Cinema (plus fonce si faible velocite)
            const float alpha = 0.5f + 0.5f * ((float) st.velocity / 127.0f);
            g.setColour (KinoColors::red.withAlpha (alpha));
            g.fillRoundedRectangle (nr, 3.0f);

            if (st.accent || s == playHead)
            {
                g.setColour (s == playHead ? KinoColors::ink : KinoColors::text);
                g.drawRoundedRectangle (nr, 3.0f, 1.5f);
            }
            if (st.slide)
            {
                g.setColour (juce::Colours::white);
                g.setFont (juce::Font (juce::FontOptions (8.5f).withStyle ("Bold")));
                g.drawText ("S", nr.toNearestInt(), juce::Justification::centred);
            }
        }
    }

    g.setColour (KinoColors::border);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
}

void PianoRollGrid::handlePointer (const juce::MouseEvent& e, bool first)
{
    const int nSteps = juce::jlimit (4, KinoProcessor::maxSteps, (int) proc.P ("steps"));
    const float gridW = (float) getWidth() - (float) labelW;
    if (e.x < labelW || gridW <= 1.0f) return;

    const int s = juce::jlimit (0, nSteps - 1, (int) (((float) (e.x - labelW) / gridW) * nSteps));
    const int row = juce::jlimit (0, numRows - 1, (int) (((float) e.y / (float) getHeight()) * numRows));
    const int deg = maxDeg - row;

    if (! first && s == lastStep && deg == lastDeg) return;
    lastStep = s; lastDeg = deg;

    auto st = proc.getStep (s);
    if (e.mods.isPopupMenu())
    {
        st.active = false;
        proc.setStep (s, st);
    }
    else
    {
        if (first) erasing = (st.active && st.degree == deg);
        if (erasing) st.active = false;
        else { st.active = true; st.degree = deg; }
        proc.setStep (s, st);
    }
    repaint();
    if (auto* p = getParentComponent()) p->repaint();
}

void PianoRollGrid::mouseDown (const juce::MouseEvent& e) { lastStep = -1; lastDeg = -99; handlePointer (e, true); }
void PianoRollGrid::mouseDrag (const juce::MouseEvent& e) { handlePointer (e, false); }

// =============================================================== GoofyStepStrip
GoofyStepStrip::GoofyStepStrip (KinoProcessor& p) : proc (p) {}

void GoofyStepStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const int lw = PianoRollGrid::labelW;
    const int nSteps = juce::jlimit (4, KinoProcessor::maxSteps, (int) proc.P ("steps"));
    const int playHead = proc.currentStep.load();
    const float gridW = r.getWidth() - (float) lw;
    const float cellW = gridW / (float) nSteps;

    const float velH = r.getHeight() * 0.46f;
    const float rowH = (r.getHeight() - velH) / 3.0f;

    // Labels de gauche
    g.setColour (KinoColors::textDim);
    g.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
    g.drawText ("VEL", 2, 0, lw - 6, (int) velH, juce::Justification::centredRight);
    g.drawText ("RATCH", 2, (int) velH, lw - 6, (int) rowH, juce::Justification::centredRight);
    g.drawText ("MOD", 2, (int) (velH + rowH), lw - 6, (int) rowH, juce::Justification::centredRight);
    g.drawText ("PROB", 2, (int) (velH + rowH * 2.0f), lw - 6, (int) rowH, juce::Justification::centredRight);

    juce::Path intCurvePath;

    for (int s = 0; s < nSteps; ++s)
    {
        const float x = (float) lw + s * cellW;
        const auto st = proc.getStep (s);
        const float mult = proc.intensityMultiplierForStep (s, nSteps);

        // 1. Barre de velocite
        auto vr = juce::Rectangle<float> (x + 2.0f, 2.0f, std::max (4.0f, cellW - 4.0f), velH - 4.0f);
        g.setColour (KinoColors::bg);
        g.fillRoundedRectangle (vr, 2.0f);
        if (st.active)
        {
            const float bh = vr.getHeight() * ((float) st.velocity / 127.0f);
            auto bar = vr.withTop (vr.getBottom() - bh);
            g.setColour (s == playHead ? KinoColors::red : KinoColors::red.withAlpha (0.78f));
            g.fillRoundedRectangle (bar, 2.0f);
        }

        // Courbe d'automation d'intensite (ex: Snare Roll Crescendo!)
        const float cy = vr.getBottom() - vr.getHeight() * juce::jlimit (0.0f, 1.0f, mult);
        if (s == 0) intCurvePath.startNewSubPath (x + cellW * 0.5f, cy);
        else intCurvePath.lineTo (x + cellW * 0.5f, cy);

        // 2. Ratchet (x1..x4)
        auto rr = juce::Rectangle<float> (x + 2.0f, velH + 1.5f, std::max (4.0f, cellW - 4.0f), rowH - 3.0f);
        g.setColour (st.ratchet > 1 ? KinoColors::redSoft : KinoColors::bg);
        g.fillRoundedRectangle (rr, 2.5f);
        g.setColour (st.ratchet > 1 ? KinoColors::red : KinoColors::textDim);
        g.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
        g.drawText ("x" + juce::String (st.ratchet), rr.toNearestInt(), juce::Justification::centred);

        // 3. Modificateur Goofy (SLD / ACC / +1O / -1O)
        auto mr = juce::Rectangle<float> (x + 2.0f, velH + rowH + 1.5f, std::max (4.0f, cellW - 4.0f), rowH - 3.0f);
        juce::String modTxt = "-";
        bool hasMod = st.slide || st.accent || st.octJump != 0;
        if (st.accent) modTxt = "ACC";
        else if (st.slide) modTxt = "SLD";
        else if (st.octJump > 0) modTxt = "+1O";
        else if (st.octJump < 0) modTxt = "-1O";
        g.setColour (hasMod ? KinoColors::red : KinoColors::bg);
        g.fillRoundedRectangle (mr, 2.5f);
        g.setColour (hasMod ? juce::Colours::white : KinoColors::textDim);
        g.drawText (modTxt, mr.toNearestInt(), juce::Justification::centred);

        // 4. Probabilite (%)
        auto pr = juce::Rectangle<float> (x + 2.0f, velH + rowH * 2.0f + 1.5f, std::max (4.0f, cellW - 4.0f), rowH - 3.0f);
        g.setColour (st.prob < 100 ? KinoColors::redSoft : KinoColors::bg);
        g.fillRoundedRectangle (pr, 2.5f);
        g.setColour (st.prob < 100 ? KinoColors::red : KinoColors::textDim);
        g.drawText (juce::String (st.prob) + "%", pr.toNearestInt(), juce::Justification::centred);
    }

    // Ligne d'automation d'intensite par-dessus les barres de velocite
    g.setColour (KinoColors::ink.withAlpha (0.75f));
    g.strokePath (intCurvePath, juce::PathStrokeType (1.8f));
}

void GoofyStepStrip::handleVelDrag (const juce::MouseEvent& e)
{
    const int lw = PianoRollGrid::labelW;
    const int nSteps = juce::jlimit (4, KinoProcessor::maxSteps, (int) proc.P ("steps"));
    const float gridW = (float) getWidth() - (float) lw;
    if (e.x < lw || gridW <= 1.0f) return;

    const int s = juce::jlimit (0, nSteps - 1, (int) (((float) (e.x - lw) / gridW) * nSteps));
    const float velH = (float) getHeight() * 0.46f;
    const float norm = juce::jlimit (0.05f, 1.0f, 1.0f - (float) e.y / velH);
    auto st = proc.getStep (s);
    st.velocity = juce::jlimit (10, 127, (int) std::round (norm * 127.0f));
    proc.setStep (s, st);
    repaint();
}

void GoofyStepStrip::mouseDown (const juce::MouseEvent& e)
{
    const int lw = PianoRollGrid::labelW;
    const int nSteps = juce::jlimit (4, KinoProcessor::maxSteps, (int) proc.P ("steps"));
    const float gridW = (float) getWidth() - (float) lw;
    if (e.x < lw || gridW <= 1.0f) return;

    const int s = juce::jlimit (0, nSteps - 1, (int) (((float) (e.x - lw) / gridW) * nSteps));
    const float velH = (float) getHeight() * 0.46f;
    const float rowH = ((float) getHeight() - velH) / 3.0f;

    draggingVel = (e.y < velH);
    if (draggingVel)
    {
        handleVelDrag (e);
        return;
    }

    auto st = proc.getStep (s);
    if (e.y < velH + rowH)
    {
        st.ratchet = (st.ratchet % 4) + 1;
    }
    else if (e.y < velH + rowH * 2.0f)
    {
        // Cycle: None -> ACC -> SLD -> +1O -> -1O -> None
        if (! st.accent && ! st.slide && st.octJump == 0) st.accent = true;
        else if (st.accent) { st.accent = false; st.slide = true; }
        else if (st.slide) { st.slide = false; st.octJump = 1; }
        else if (st.octJump == 1) st.octJump = -1;
        else st.octJump = 0;
    }
    else
    {
        if (st.prob == 100) st.prob = 75;
        else if (st.prob == 75) st.prob = 50;
        else if (st.prob == 50) st.prob = 25;
        else st.prob = 100;
    }
    proc.setStep (s, st);
    repaint();
}

void GoofyStepStrip::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingVel) handleVelDrag (e);
}

// =============================================================== KinoEditor
KinoEditor::KinoEditor (KinoProcessor& p)
    : AudioProcessorEditor (&p), proc (p), dragMidi (p), pianoRoll (p), stepStrip (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    playBtn.setClickingTogglesState (true);
    addAndMakeVisible (playBtn);
    aPlay = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "run", playBtn);

    kBpm   = std::make_unique<KinoKnob> (p.apvts, "bpm", "BPM");
    kSteps = std::make_unique<KinoKnob> (p.apvts, "steps", "STEPS");
    kSwing = std::make_unique<KinoKnob> (p.apvts, "swing", "SWING");
    addAndMakeVisible (*kBpm); addAndMakeVisible (*kSteps); addAndMakeVisible (*kSwing);

    if (auto* pr = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter ("rate")))
        cRate.addItemList (pr->choices, 1);
    addAndMakeVisible (cRate);
    aRate = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "rate", cRate);

    modelBtn.onClick = [this] { showModelMenu(); };
    prevModelBtn.onClick = [this] { proc.loadModel ((proc.currentModelIndex.load() - 1 + (int) proc.getModels().size()) % (int) proc.getModels().size()); updateModelButtonText(); repaint(); };
    nextModelBtn.onClick = [this] { proc.loadModel ((proc.currentModelIndex.load() + 1) % (int) proc.getModels().size()); updateModelButtonText(); repaint(); };
    addAndMakeVisible (modelBtn); addAndMakeVisible (prevModelBtn); addAndMakeVisible (nextModelBtn);
    updateModelButtonText();

    addAndMakeVisible (dragMidi);
    saveMidiBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Exporter la sequence en MIDI",
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Kino_Pattern.mid"), "*.mid");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) proc.exportMidiFile (fc.getResult()); });
    };
    clearBtn.onClick = [this] { proc.clearPattern(); repaint(); };
    addAndMakeVisible (saveMidiBtn); addAndMakeVisible (clearBtn);

    addAndMakeVisible (pianoRoll);
    addAndMakeVisible (stepStrip);

    // Right cards controls
    auto fillCombo = [&] (juce::ComboBox& cb, const char* id)
    {
        if (auto* pr = dynamic_cast<juce::AudioParameterChoice*> (p.apvts.getParameter (id)))
            cb.addItemList (pr->choices, 1);
        addAndMakeVisible (cb);
    };
    fillCombo (cRoot, "root");
    fillCombo (cScale, "scale");
    fillCombo (cChords, "chordMode");
    fillCombo (cIntCurve, "intCurve");
    aRoot     = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "root", cRoot);
    aScale    = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "scale", cScale);
    aChords   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "chordMode", cChords);
    aIntCurve = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, "intCurve", cIntCurve);

    liveTransBtn.setClickingTogglesState (true);
    addAndMakeVisible (liveTransBtn);
    aLiveTrans = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "liveTrans", liveTransBtn);

    kOctave    = std::make_unique<KinoKnob> (p.apvts, "octave", "OCTAVE");
    kIntensity = std::make_unique<KinoKnob> (p.apvts, "intensity", "INTENSITE");
    kEucPulses = std::make_unique<KinoKnob> (p.apvts, "eucPulses", "PULSES");
    kEucRot    = std::make_unique<KinoKnob> (p.apvts, "eucRot", "ROTAT.");
    addAndMakeVisible (*kOctave); addAndMakeVisible (*kIntensity);
    addAndMakeVisible (*kEucPulses); addAndMakeVisible (*kEucRot);

    euclidBtn.onClick = [this] { proc.applyEuclidean ((int) proc.P ("eucPulses"), (int) proc.P ("eucRot")); repaint(); };
    mutateBtn.onClick = [this] { proc.mutatePattern(); repaint(); };
    addAndMakeVisible (euclidBtn); addAndMakeVisible (mutateBtn);

    sawOnBtn.setClickingTogglesState (true);
    addAndMakeVisible (sawOnBtn);
    aSawOn = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "sawOn", sawOnBtn);
    kSawVol   = std::make_unique<KinoKnob> (p.apvts, "sawVol", "VOL");
    kSawCut   = std::make_unique<KinoKnob> (p.apvts, "sawCut", "CUTOFF");
    kSawDecay = std::make_unique<KinoKnob> (p.apvts, "sawDecay", "DECAY");
    addAndMakeVisible (*kSawVol); addAndMakeVisible (*kSawCut); addAndMakeVisible (*kSawDecay);

    // 4 Goofy 3D Spectators (Preset shortcuts + rhythm animation)
    spectators.push_back (std::make_unique<SpectatorAvatar> (0, "PSY BASS",   [this] { proc.loadModel (0); updateModelButtonText(); repaint(); }));
    spectators.push_back (std::make_unique<SpectatorAvatar> (1, "ACID 303",   [this] { proc.loadModel (3); updateModelButtonText(); repaint(); }));
    spectators.push_back (std::make_unique<SpectatorAvatar> (2, "SNARE ROLL", [this] { proc.loadModel (9); updateModelButtonText(); repaint(); }));
    spectators.push_back (std::make_unique<SpectatorAvatar> (3, "GOOFY FILL", [this] { proc.loadModel (13); updateModelButtonText(); repaint(); }));
    for (auto& sp : spectators) addAndMakeVisible (*sp);

    // Light Clean UI Keyboard
    keyboard.setAvailableRange (36, 84);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, KinoColors::card);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, KinoColors::text);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, KinoColors::border);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, KinoColors::redSoft);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, KinoColors::red);
    addAndMakeVisible (keyboard);

    setResizable (true, true);
    setResizeLimits (1020, 640, 1680, 1080);
    setSize (1180, 730);
    startTimerHz (30);
}

KinoEditor::~KinoEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void KinoEditor::updateModelButtonText()
{
    const auto& ms = proc.getModels();
    const int idx = proc.currentModelIndex.load();
    if (idx >= 0 && idx < (int) ms.size())
        modelBtn.setButtonText (ms[(size_t) idx].category.toUpperCase() + " : " + ms[(size_t) idx].name);
}

void KinoEditor::showModelMenu()
{
    juce::PopupMenu menu;
    const auto& ms = proc.getModels();
    juce::StringArray cats;
    for (auto& m : ms) if (! cats.contains (m.category)) cats.add (m.category);

    for (auto& c : cats)
    {
        juce::PopupMenu sub;
        for (int i = 0; i < (int) ms.size(); ++i)
            if (ms[(size_t) i].category == c)
                sub.addItem (i + 1, ms[(size_t) i].name, true, i == proc.currentModelIndex.load());
        menu.addSubMenu (c, sub);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (modelBtn), [this] (int r)
    {
        if (r > 0) { proc.loadModel (r - 1); updateModelButtonText(); repaint(); }
    });
}

void KinoEditor::timerCallback()
{
    const float p = proc.visualPulse.load();
    for (auto& sp : spectators) sp->setPulse (p);
    pianoRoll.repaint();
    stepStrip.repaint();
}

void KinoEditor::drawCard (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto rf = r.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.05f));
    g.fillRoundedRectangle (rf.translated (0.0f, 3.0f), 8.0f);

    g.setColour (KinoColors::card);
    g.fillRoundedRectangle (rf, 8.0f);
    g.setColour (KinoColors::border);
    g.drawRoundedRectangle (rf.reduced (0.5f), 8.0f, 1.0f);

    // Petit accent rouge cinema + titre technique
    g.setColour (KinoColors::red);
    g.fillRoundedRectangle (rf.getX() + 12.0f, rf.getY() + 10.0f, 4.0f, 12.0f, 2.0f);
    g.setColour (KinoColors::text);
    g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText (title, r.getX() + 22, r.getY() + 8, r.getWidth() - 30, 16, juce::Justification::centredLeft);
}

void KinoEditor::paint (juce::Graphics& g)
{
    g.fillAll (KinoColors::bg);

    // Top bar card
    auto topR = juce::Rectangle<int> (12, 10, getWidth() - 24, 58);
    g.setColour (juce::Colours::black.withAlpha (0.04f));
    g.fillRoundedRectangle (topR.toFloat().translated (0.0f, 2.0f), 8.0f);
    g.setColour (KinoColors::card);
    g.fillRoundedRectangle (topR.toFloat(), 8.0f);
    g.setColour (KinoColors::border);
    g.drawRoundedRectangle (topR.toFloat().reduced (0.5f), 8.0f, 1.0f);

    // Logo KINO
    g.setColour (KinoColors::red);
    g.setFont (juce::Font (juce::FontOptions (24.0f).withStyle ("Bold")));
    g.drawText ("KINO", 24, 15, 90, 26, juce::Justification::left);
    g.setColour (KinoColors::textDim);
    g.setFont (juce::Font (juce::FontOptions (9.5f).withStyle ("Bold")));
    g.drawText ("GOOFY MIDI CREATOR", 24, 42, 140, 14, juce::Justification::left);

    drawCard (g, cardSeq,   "PIANO ROLL & STEP AUTOMATION (CLIC G: NOTE  |  CLIC D: EFFACER)");
    drawCard (g, cardScale, "NOTE DE BASE, GAMME & ACCORDS");
    drawCard (g, cardGen,   "GENERATEURS, INTENSITE & AUTOMATION");
    drawCard (g, cardSaw,   "PRE-ECOUTE INTERNE (SAW WAVE)");
    drawCard (g, cardCrew,  "LES SPECTATEURS 3D (PRESETS GOOFY)");
}

void KinoEditor::resized()
{
    const int W = getWidth(), H = getHeight();

    // Top bar layout
    int tx = 164;
    playBtn.setBounds (tx, 24, 56, 30); tx += 64;
    kBpm->setBounds   (tx, 14, 52, 50); tx += 54;
    kSteps->setBounds (tx, 14, 52, 50); tx += 54;
    kSwing->setBounds (tx, 14, 52, 50); tx += 56;
    cRate.setBounds   (tx, 25, 68, 28); tx += 76;

    prevModelBtn.setBounds (tx, 25, 26, 28); tx += 28;
    const int rightBtnsW = 286;
    const int modelW = std::max (130, W - tx - rightBtnsW - 38);
    modelBtn.setBounds (tx, 25, modelW, 28); tx += modelW + 2;
    nextModelBtn.setBounds (tx, 25, 26, 28);

    clearBtn.setBounds    (W - 294, 25, 60, 28);
    saveMidiBtn.setBounds (W - 228, 25, 78, 28);
    dragMidi.setBounds    (W - 144, 24, 122, 30);

    // Main split: Left (PianoRoll + StepStrip), Right (Modular Cards)
    const int mainY = 78;
    const int keyH = 62;
    const int mainH = H - mainY - keyH - 16;
    const int sideW = 350;
    const int seqW = W - sideW - 36;

    cardSeq = { 12, mainY, seqW, mainH };
    auto seqIn = cardSeq.reduced (10, 8).withTrimmedTop (24);
    const int stripH = 112;
    pianoRoll.setBounds (seqIn.removeFromTop (seqIn.getHeight() - stripH - 6));
    seqIn.removeFromTop (6);
    stepStrip.setBounds (seqIn);

    // Right cards stacked cleanly
    const int sx = 12 + seqW + 12;
    const int c1H = 122, c2H = 160, c3H = 104;
    const int c4H = std::max (105, mainH - c1H - c2H - c3H - 24);

    // Card 1: Scale & Root
    cardScale = { sx, mainY, sideW, c1H };
    auto r1 = cardScale.reduced (10, 6).withTrimmedTop (24);
    cRoot.setBounds  (r1.getX(), r1.getY() + 4, 66, 26);
    cScale.setBounds (r1.getX() + 72, r1.getY() + 4, 134, 26);
    liveTransBtn.setButtonText ("TRANS");
    liveTransBtn.setBounds (r1.getX(), r1.getY() + 38, 66, 26);
    cChords.setBounds      (r1.getX() + 72, r1.getY() + 38, 134, 26);
    kOctave->setBounds (r1.getRight() - 62, r1.getY() - 2, 58, 68);

    // Card 2: Generators & Intensity Automation
    cardGen = { sx, mainY + c1H + 8, sideW, c2H };
    auto r2 = cardGen.reduced (10, 6).withTrimmedTop (24);
    kIntensity->setBounds (r2.getX(), r2.getY(), 66, 72);
    cIntCurve.setBounds   (r2.getX() + 72, r2.getY() + 20, 122, 28);
    kEucPulses->setBounds (r2.getRight() - 130, r2.getY(), 62, 72);
    kEucRot->setBounds    (r2.getRight() - 64, r2.getY(), 62, 72);
    euclidBtn.setBounds   (r2.getX() + 4, r2.getBottom() - 32, (r2.getWidth() - 14) / 2, 28);
    mutateBtn.setBounds   (r2.getX() + (r2.getWidth() - 14) / 2 + 10, r2.getBottom() - 32, (r2.getWidth() - 14) / 2, 28);

    // Card 3: Internal Saw Wave
    cardSaw = { sx, mainY + c1H + c2H + 16, sideW, c3H };
    auto r3 = cardSaw.reduced (10, 4).withTrimmedTop (22);
    sawOnBtn.setBounds (cardSaw.getRight() - 52, cardSaw.getY() + 6, 40, 20);
    const int sw = r3.getWidth() / 3;
    kSawVol->setBounds   (r3.getX(), r3.getY(), sw, r3.getHeight());
    kSawCut->setBounds   (r3.getX() + sw, r3.getY(), sw, r3.getHeight());
    kSawDecay->setBounds (r3.getX() + sw * 2, r3.getY(), sw, r3.getHeight());

    // Card 4: Goofy 3D Spectators
    cardCrew = { sx, mainY + c1H + c2H + c3H + 24, sideW, c4H };
    auto r4 = cardCrew.reduced (8, 6).withTrimmedTop (24);
    const int avW = r4.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
        spectators[(size_t) i]->setBounds (r4.getX() + i * avW, r4.getY(), avW, r4.getHeight());

    // Bottom MIDI Keyboard
    keyboard.setBounds (12, H - keyH - 8, W - 24, keyH);
}
