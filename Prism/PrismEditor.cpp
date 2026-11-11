#include "PrismEditor.h"
using namespace PsyUI;

// =============================================================== Forme d'onde
WaveDisplay::WaveDisplay (PrismProcessor& p) : proc (p) {}

void WaveDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel); g.fillRoundedRectangle (r, 6.0f);
    auto head = r.removeFromTop (24.0f);
    g.setColour (panelHi.withAlpha (0.7f));
    g.fillRoundedRectangle (head.getX() + 1.0f, head.getY() + 1.0f, head.getWidth() - 2.0f, head.getHeight(), 5.0f);
    g.setColour (red);
    g.fillRoundedRectangle (head.getX() + 8.0f, head.getY() + 6.0f, 3.0f, 12.0f, 1.5f);
    g.setColour (line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setColour (text); g.setFont (juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")));
    auto name = proc.getUserName();
    g.drawText ("FORME D'ONDE  (Glisser audio ou LOAD) " + (name.isEmpty() ? "" : " - " + name), 18, 4, (int) r.getWidth() - 24, 16, juce::Justification::centredLeft);

    const auto* w = proc.currentWave();
    if (w == nullptr || w->empty()) return;
    const float x0 = 10.f, x1 = (float) getWidth() - 10.f, W = x1 - x0, cy = 26.f + (getHeight() - 32.f) * 0.5f, H = (getHeight() - 32.f) * 0.42f;
    const float s = proc.P ("start"), e = proc.P ("end");
    g.setColour (panelHi.withAlpha (0.65f)); g.fillRoundedRectangle (x0 + s * W, 26.f, (e - s) * W, (float) getHeight() - 32.f, 3.0f);
    juce::Path p; p.startNewSubPath (x0, cy);
    const int N = (int) w->size(), pts = (int) W;
    for (int i = 0; i < pts; ++i)
    {
        const int idx = std::min (N - 1, (int) ((double) i / pts * N));
        p.lineTo (x0 + (float) i, cy - (*w)[(size_t) idx] * H);
    }
    g.setColour (red.withAlpha (0.3f)); g.strokePath (p, juce::PathStrokeType (3.0f));
    g.setColour (redHot); g.strokePath (p, juce::PathStrokeType (1.5f));
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawVerticalLine ((int) (x0 + s * W), 26.f, (float) getHeight() - 6.f);
    g.drawVerticalLine ((int) (x0 + e * W), 26.f, (float) getHeight() - 6.f);
    const float ph = s + proc.playRatio * (e - s);
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.drawVerticalLine ((int) (x0 + juce::jlimit (0.f, 1.f, ph) * W), 26.f, (float) getHeight() - 6.f);
}

void WaveDisplay::drag (float u, bool first)
{
    const float s = proc.P ("start"), e = proc.P ("end");
    if (first) moveEnd = std::abs (u - e) < std::abs (u - s);
    if (auto* p = proc.apvts.getParameter (moveEnd ? "end" : "start"))
        p->setValueNotifyingHost (p->convertTo0to1 (moveEnd ? juce::jlimit (s + 0.03f, 1.f, u) : juce::jlimit (0.f, e - 0.03f, u)));
}
bool WaveDisplay::isInterestedInFileDrag (const juce::StringArray& f)
{
    for (auto& n : f) if (juce::File (n).hasFileExtension ("wav;aif;aiff;mp3;flac;ogg")) return true;
    return false;
}
void WaveDisplay::filesDropped (const juce::StringArray& f, int, int) { if (f.size() > 0) proc.loadUserSample (juce::File (f[0])); }

// =============================================================== Courbe de filtre
void FilterDisplay::setFromMouse (const juce::MouseEvent& e)
{
    const float x0 = 10.f, W = std::max (1.f, (float) getWidth() - 20.f);
    const float y0 = 28.f, H = std::max (1.f, (float) getHeight() - 34.f);
    const float u = juce::jlimit (0.f, 1.f, ((float) e.x - x0) / W);
    const float v = juce::jlimit (0.f, 1.f, 1.f - ((float) e.y - y0) / H);
    const float fc = 20.f * std::pow (1000.f, u);
    const bool f2 = e.mods.isPopupMenu();
    if (auto* pc = proc.apvts.getParameter (f2 ? "f2Cut" : "f1Cut")) pc->setValueNotifyingHost (pc->convertTo0to1 (fc));
    if (auto* pr = proc.apvts.getParameter (f2 ? "f2Res" : "f1Res")) pr->setValueNotifyingHost (pr->convertTo0to1 (v));
}

float FilterDisplay::magDb (int type, float f, float fc, float res) const
{
    const float x = f / std::max (20.f, fc), Q = 0.7f + res * res * 14.f;
    auto svf = [&] (float n) { float d = (1.f - x * x); return n / std::sqrt (d * d + (x / Q) * (x / Q) + 1e-9f); };
    float m = 1.f;
    switch (type)
    {
        case 0: m = svf (1.f); break;
        case 1: m = svf (1.f); m *= m; break;
        case 2: m = svf (x * x); break;
        case 3: m = svf (x * x); m *= m; break;
        case 4: m = svf (x / Q) * Q * 0.6f; break;
        case 5: m = svf (x / Q) * Q * 0.6f; m *= m; break;
        case 6: m = svf (std::abs (1.f - x * x)); break;
        case 7: { float l = 1.f / (1.f + x * x); m = l * l + res * 3.5f * std::exp (-std::pow ((x - 1.f) * 4.f, 2.f)); break; }
        case 8: case 9: m = 0.6f + 0.5f * (type == 8 ? 1.f : -1.f) * res * std::cos (6.283f * x); break;
        default: m = 0.25f + 0.9f * (std::exp (-std::pow ((x - 0.6f) * 3.f, 2.f)) + std::exp (-std::pow ((x - 1.7f) * 3.f, 2.f))); break;
    }
    return 20.f * std::log10 (std::max (0.001f, m));
}

void FilterDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel); g.fillRoundedRectangle (r, 6.0f);
    auto head = r.removeFromTop (24.0f);
    g.setColour (panelHi.withAlpha (0.7f));
    g.fillRoundedRectangle (head.getX() + 1.0f, head.getY() + 1.0f, head.getWidth() - 2.0f, head.getHeight(), 5.0f);
    g.setColour (red);
    g.fillRoundedRectangle (head.getX() + 8.0f, head.getY() + 6.0f, 3.0f, 12.0f, 1.5f);
    g.setColour (line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setColour (text); g.setFont (juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")));
    g.drawText ("COURBE DES FILTRES  (Clic G: F1 / Clic D: F2)", 18, 4, (int) r.getWidth() - 24, 16, juce::Justification::centredLeft);

    const float x0 = 10.f, W = (float) getWidth() - 20.f, y0 = 28.f, H = (float) getHeight() - 34.f;
    g.setColour (line); g.drawHorizontalLine ((int) (y0 + H * 0.4f), x0, x0 + W);
    const bool f1 = proc.P ("f1On") > 0.5f, f2 = proc.P ("f2On") > 0.5f, ser = (int) proc.P ("routing") == 0;
    const int t1 = (int) proc.P ("f1Type"), t2 = (int) proc.P ("f2Type");
    const float c1 = proc.modCut1, c2 = proc.modCut2, r1 = proc.P ("f1Res"), r2 = proc.P ("f2Res");
    juce::Path p, fill;
    for (int i = 0; i <= (int) W; ++i)
    {
        const float u = (float) i / W, f = 20.f * std::pow (1000.f, u);
        float db = 0;
        if (ser) { if (f1) db += magDb (t1, f, c1, r1); if (f2) db += magDb (t2, f, c2, r2); }
        else
        {
            const float m1 = f1 ? std::pow (10.f, magDb (t1, f, c1, r1) / 20.f) : 0.f;
            const float m2 = f2 ? std::pow (10.f, magDb (t2, f, c2, r2) / 20.f) : 0.f;
            db = 20.f * std::log10 (std::max (0.001f, (m1 + m2) * 0.7f));
        }
        const float y = y0 + H * (0.4f - juce::jlimit (-36.f, 24.f, db) / 60.f);
        if (i == 0) { p.startNewSubPath (x0, y); fill.startNewSubPath (x0, y0 + H); fill.lineTo (x0, y); }
        else { p.lineTo (x0 + (float) i, y); fill.lineTo (x0 + (float) i, y); }
    }
    fill.lineTo (x0 + W, y0 + H); fill.closeSubPath();
    g.setColour (redDark.withAlpha (0.45f)); g.fillPath (fill);
    g.setColour (redHot); g.strokePath (p, juce::PathStrokeType (2.2f));
}

// =============================================================== Sequenceur de filtre
void SeqGrid::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel); g.fillRoundedRectangle (r, 6.0f);
    auto head = r.removeFromTop (24.0f);
    g.setColour (panelHi.withAlpha (0.7f));
    g.fillRoundedRectangle (head.getX() + 1.0f, head.getY() + 1.0f, head.getWidth() - 2.0f, head.getHeight(), 5.0f);
    g.setColour (red);
    g.fillRoundedRectangle (head.getX() + 8.0f, head.getY() + 6.0f, 3.0f, 12.0f, 1.5f);
    g.setColour (line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6.0f, 1.0f);
    g.setColour (text); g.setFont (juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")));
    g.drawText ("SEQUENCEUR DE FILTRE 32 PAS  (Dessiner a la souris / Clic D = 0)", 18, 4, (int) r.getWidth() - 24, 16, juce::Justification::centredLeft);

    const int steps = proc.getSteps(), cur = proc.curStep;
    const float x0 = 10.f, W = (float) getWidth() - 20.f, y0 = 28.f, H = (float) getHeight() - 34.f, cy = y0 + H * 0.5f, cw = W / PrismProcessor::maxSteps;
    g.setColour (line); g.drawHorizontalLine ((int) cy, x0, x0 + W);
    for (int i = 0; i < PrismProcessor::maxSteps; ++i)
    {
        const bool on = i < steps;
        auto bx = x0 + i * cw + 1.f;
        g.setColour (on ? ((i / 4) % 2 == 0 ? panelHi : panel.brighter (0.02f)) : bg);
        g.fillRoundedRectangle (bx, y0, cw - 2.f, H, 2.0f);
        if (on)
        {
            const float v = proc.getStep (i), bh = std::abs (v) * (H * 0.48f);
            g.setColour (v >= 0 ? redHot : redDark.brighter (0.3f));
            g.fillRoundedRectangle (bx, v >= 0 ? cy - bh : cy, cw - 2.f, std::max (2.f, bh), 2.0f);
            if (i == cur) { g.setColour (juce::Colours::white.withAlpha (0.75f)); g.drawRoundedRectangle (bx, y0, cw - 2.f, H, 2.0f, 1.5f); }
        }
    }
}

void SeqGrid::setAt (const juce::MouseEvent& e)
{
    const float x0 = 10.f, W = (float) getWidth() - 20.f, y0 = 28.f, H = (float) getHeight() - 34.f;
    const int i = (int) (((float) e.x - x0) / (W / PrismProcessor::maxSteps));
    if (i < 0 || i >= PrismProcessor::maxSteps) return;
    const float v = e.mods.isPopupMenu() ? 0.f : juce::jlimit (-1.f, 1.f, 1.f - 2.f * ((float) e.y - y0) / H);
    proc.setStep (i, v); repaint();
}

// =============================================================== Editeur Prism
Panel* PrismEditor::addPanel (const juce::String& title, const juce::String& power)
{
    panels.push_back (std::make_unique<Panel> (proc.apvts, title, power));
    addChildComponent (*panels.back()); return panels.back().get();
}

PrismEditor::PrismEditor (PrismProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      presetBar (p.apvts, p.presets, "PrismFilter", { "vol" }),
      master (p.apvts, "vol", "MASTER"),
      tabs ({ "1. SAMPLE & FILTRES", "2. MODULATIONS (ENV / LFO / SEQ)", "3. EFFETS & ESPACE" }, [this] (int idx) { setTab (idx); }),
      wave (p), filterDisp (p), seqGrid (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    for (auto* c : { (juce::Component*) &presetBar, (juce::Component*) &master, (juce::Component*) &tabs,
                     (juce::Component*) &loadBtn, (juce::Component*) &shapeBtn,
                     (juce::Component*) &wave, (juce::Component*) &filterDisp, (juce::Component*) &seqGrid })
        addAndMakeVisible (c);

    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Charger un sample", juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.loadUserSample (fc.getResult()); });
    };
    shapeBtn.onClick = [this]
    {
        juce::PopupMenu m;
        const char* n[8] = { "Rampe montante", "Rampe descendante", "Triangle", "Sinus", "Escalier", "Trance gate", "Aleatoire", "Remettre a zero" };
        for (int i = 0; i < 8; ++i) m.addItem (i + 1, n[i]);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (shapeBtn), [this] (int r) { if (r > 0) { proc.setWave (r - 1); seqGrid.repaint(); } });
    };

    // --- FENETRE 1 : SAMPLE, GRANULAIRE, FILTRE 1, FILTRE 2, ROUTAGE (panels 0..4) ---
    auto* smp = addPanel ("LECTEUR DE SAMPLE & PITCH");
    smp->addCombo ("src", "SOURCE", 136); smp->addCombo ("trig", "MODE MIDI", 136); smp->addCombo ("loopMode", "BOUCLE", 104);
    smp->addToggle ("rev", "REVERSE", 72); smp->addKnob ("start", "START"); smp->addKnob ("end", "END");
    smp->addKnob ("speed", "SPEED"); smp->addKnob ("root", "ROOT"); smp->addKnob ("semi", "SEMI");
    smp->addKnob ("fine", "FINE"); smp->addKnob ("glide", "GLIDE");

    auto* grn = addPanel ("MOTEUR GRANULAIRE", "grnOn");
    grn->addKnob ("grnSize", "TAILLE"); grn->addKnob ("grnDens", "DENSITE");
    grn->addKnob ("grnJit", "JITTER"); grn->addKnob ("grnSpread", "STEREO"); grn->addKnob ("grnPitch", "PITCH");

    auto* f1 = addPanel ("FILTRE 1 MULTI-MODE", "f1On");
    f1->addCombo ("f1Type", "TOPOLOGIE", 110); f1->addKnob ("f1Cut", "CUTOFF"); f1->addKnob ("f1Res", "RESO");
    f1->addKnob ("f1Drive", "DRIVE"); f1->addKnob ("f1Morph", "VOYELLE"); f1->addKnob ("f1Key", "KEY TRK"); f1->addKnob ("f1Pan", "PAN");

    auto* f2 = addPanel ("FILTRE 2 MULTI-MODE", "f2On");
    f2->addCombo ("f2Type", "TOPOLOGIE", 110); f2->addKnob ("f2Cut", "CUTOFF"); f2->addKnob ("f2Res", "RESO");
    f2->addKnob ("f2Drive", "DRIVE"); f2->addKnob ("f2Morph", "VOYELLE"); f2->addKnob ("f2Key", "KEY TRK"); f2->addKnob ("f2Pan", "PAN");

    auto* rt = addPanel ("ROUTAGE FILTRES");
    rt->addCombo ("routing", "ARCHITECTURE", 120); rt->addKnob ("fMix", "BALANCE 1/2");

    // --- FENETRE 2 : MODULATIONS (ENVELOPPES, LFO 1, LFO 2, SEQUENCEUR) (panels 5..8) ---
    auto* env = addPanel ("ENVELOPPES ADSR (AMPLI & FILTRES)");
    env->addKnob ("aAtk", "AMP A"); env->addKnob ("aDec", "AMP D"); env->addKnob ("aSus", "AMP S"); env->addKnob ("aRel", "AMP R");
    env->addKnob ("fAtk", "FLT A"); env->addKnob ("fDec", "FLT D"); env->addKnob ("fSus", "FLT S"); env->addKnob ("fRel", "FLT R");
    env->addKnob ("fe1", "ENV > F1"); env->addKnob ("fe2", "ENV > F2"); env->addKnob ("velMod", "VEL > FLT");

    auto* sq = addPanel ("SEQUENCEUR DE FILTRE & TRANCE GATE", "seqOn");
    sq->addKnob ("seqSteps", "PAS"); sq->addCombo ("seqDiv", "VITESSE", 96); sq->addKnob ("seqSmooth", "SMOOTH");
    sq->addKnob ("seqSwing", "SWING"); sq->addKnob ("seqC1", "SEQ > F1"); sq->addKnob ("seqC2", "SEQ > F2");
    sq->addKnob ("seqAmp", "TRANCE GATE"); sq->addKnob ("seqPitch", "SEQ > PITCH");

    for (int i = 1; i <= 2; ++i)
    {
        auto p = "l" + juce::String (i);
        auto* l = addPanel ("LFO " + juce::String (i) + " SYNCHRONISABLE");
        l->addCombo (p + "Shape", "FORME", 110); l->addToggle (p + "Sync", "SYNC", 62); l->addCombo (p + "Div", "DIV", 86);
        l->addKnob (p + "Rate", "HZ"); l->addKnob (p + "C1", "> F1"); l->addKnob (p + "C2", "> F2");
        l->addKnob (p + "Res", "> RESO"); l->addKnob (p + "Pan", "> PAN"); l->addKnob (p + "Pitch", "> PITCH");
    }

    // --- FENETRE 3 : EFFETS DEDIES (panels 9..12) ---
    auto* deg = addPanel ("DESTRUCTION & RING MODULATOR");
    deg->addKnob ("crush", "BITCRUSH"); deg->addKnob ("ring", "RING MIX"); deg->addKnob ("ringHz", "RING FREQ");

    auto* phs = addPanel ("PHASER PSYCHEDELIQUE");
    phs->addKnob ("phRate", "VITESSE"); phs->addKnob ("phDepth", "PROFONDEUR"); phs->addKnob ("phMix", "PHASER MIX");

    auto* dly = addPanel ("PSYCHEDELIC TAPE / PING-PONG DELAY");
    dly->addCombo ("dlyDiv", "TEMPS SYNC", 104); dly->addKnob ("dlyFb", "FEEDBACK"); dly->addKnob ("dlyTone", "TONE");
    dly->addKnob ("dlyWob", "WOBBLE"); dly->addKnob ("dlySpread", "PING-PONG");
    dly->addKnob ("dlySmear", "SMEAR"); dly->addKnob ("dlyMix", "DELAY MIX");

    auto* rev = addPanel ("REVERB ATMOSPHERIQUE");
    rev->addKnob ("rvSize", "TAILLE"); rev->addKnob ("rvDamp", "DAMPING"); rev->addKnob ("rvMix", "REVERB MIX");

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, redDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, red);
    keyboard.setAvailableRange (36, 84); keyboard.setKeyWidth (22.f);
    addAndMakeVisible (keyboard);

    presetBar.applyFactory (0);
    setSize (1260, 700);
    setTab (0);
    startTimerHz (30);
}

void PrismEditor::setTab (int idx)
{
    for (size_t i = 0; i < panels.size(); ++i)
    {
        bool vis = (idx == 0 && i <= 4) || (idx == 1 && i >= 5 && i <= 8) || (idx == 2 && i >= 9);
        panels[i]->setVisible (vis);
    }
    resized();
    repaint();
}

void PrismEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (redHot); g.setFont (juce::Font (juce::FontOptions (28.0f).withStyle ("Bold")));
    g.drawText ("PRISM", 16, 6, 200, 32, juce::Justification::left);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("PSYCHEDELIC FILTER SAMPLER", 18, 36, 260, 14, juce::Justification::left);
    g.setColour (line); g.drawHorizontalLine (60, 14.0f, (float) getWidth() - 14);
}

void PrismEditor::resized()
{
    const int margin = 14;
    const int totalW = getWidth() - margin * 2;

    presetBar.setBounds (260, 14, 430, 30);
    loadBtn.setBounds (704, 14, 120, 30);
    shapeBtn.setBounds (832, 14, 130, 30);
    master.setBounds (getWidth() - 78, 2, 60, 56);

    // Visualiseurs temps reel toujours visibles en haut
    const int yVis = 68, hVis = 132;
    const int wWave = (int) (totalW * 0.34f);
    const int wFlt  = (int) (totalW * 0.30f);
    const int wSeq  = totalW - wWave - wFlt - 20;
    wave.setBounds (margin, yVis, wWave, hVis);
    filterDisp.setBounds (margin + wWave + 10, yVis, wFlt, hVis);
    seqGrid.setBounds (margin + wWave + wFlt + 20, yVis, wSeq, hVis);

    // Onglets de fenetres (1. Sample & Filtres | 2. Modulations | 3. Effets)
    const int yTabs = yVis + hVis + 10;
    tabs.setBounds (margin, yTabs, totalW, 34);

    // Zone centrale aeree pour la fenetre active
    const int yBody = yTabs + 44;
    const int rowH = 162;
    const int tab = tabs.current();
    if (tab == 0)
    {
        layoutRow (margin, yBody, { panels[0].get(), panels[1].get() }, totalW, rowH);
        layoutRow (margin, yBody + rowH + 12, { panels[2].get(), panels[3].get(), panels[4].get() }, totalW, rowH);
    }
    else if (tab == 1)
    {
        layoutRow (margin, yBody, { panels[5].get(), panels[6].get() }, totalW, rowH);
        layoutRow (margin, yBody + rowH + 12, { panels[7].get(), panels[8].get() }, totalW, rowH);
    }
    else
    {
        layoutRow (margin, yBody, { panels[9].get(), panels[10].get() }, totalW, rowH);
        layoutRow (margin, yBody + rowH + 12, { panels[11].get(), panels[12].get() }, totalW, rowH);
    }

    // Clavier MIDI toujours accessible en bas
    const int yKey = yBody + (rowH + 12) * 2 + 2;
    keyboard.setBounds (margin, yKey, totalW, getHeight() - yKey - 10);
}

