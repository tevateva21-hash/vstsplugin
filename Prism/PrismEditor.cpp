#include "PrismEditor.h"
using namespace PsyUI;

// =============================================================== Forme d'onde
WaveDisplay::WaveDisplay (PrismProcessor& p) : proc (p) {}

void WaveDisplay::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    auto name = proc.getUserName();
    g.drawText ("SAMPLE  (glisser un fichier wav/aiff/mp3/flac ou cliquer LOAD)  " + (name.isEmpty() ? "" : "- " + name), 10, 4, (int) r.getWidth() - 20, 14, juce::Justification::left);

    const auto* w = proc.currentWave();
    if (w == nullptr || w->empty()) return;
    const float x0 = 8.f, x1 = r.getWidth() - 8.f, W = x1 - x0, cy = r.getCentreY() + 6.f, H = r.getHeight() * 0.36f;
    const float s = proc.P ("start"), e = proc.P ("end");
    g.setColour (panelHi); g.fillRect (x0 + s * W, 22.f, (e - s) * W, r.getHeight() - 26.f);
    juce::Path p; p.startNewSubPath (x0, cy);
    const int N = (int) w->size(), pts = (int) W;
    for (int i = 0; i < pts; ++i)
    {
        const int idx = std::min (N - 1, (int) ((double) i / pts * N));
        p.lineTo (x0 + (float) i, cy - (*w)[(size_t) idx] * H);
    }
    g.setColour (red); g.strokePath (p, juce::PathStrokeType (1.3f));
    g.setColour (redHot);
    g.drawVerticalLine ((int) (x0 + s * W), 20.f, r.getHeight() - 4.f);
    g.drawVerticalLine ((int) (x0 + e * W), 20.f, r.getHeight() - 4.f);
    const float ph = s + proc.playRatio * (e - s);
    g.setColour (juce::Colours::white.withAlpha (0.7f));
    g.drawVerticalLine ((int) (x0 + juce::jlimit (0.f, 1.f, ph) * W), 20.f, r.getHeight() - 4.f);
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
    g.setColour (panel); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("REPONSE DES FILTRES (temps reel)", 10, 4, 260, 14, juce::Justification::left);

    const float x0 = 8.f, W = r.getWidth() - 16.f, y0 = 22.f, H = r.getHeight() - 28.f;
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
    g.setColour (redHot); g.strokePath (p, juce::PathStrokeType (2.0f));
}

// =============================================================== Sequenceur de filtre
void SeqGrid::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (panel); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (line); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 1.0f);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("SEQUENCEUR DE FILTRE (dessiner a la souris, clic droit = 0)", 10, 4, 340, 14, juce::Justification::left);

    const int steps = proc.getSteps(), cur = proc.curStep;
    const float x0 = 8.f, W = r.getWidth() - 16.f, y0 = 22.f, H = r.getHeight() - 28.f, cy = y0 + H * 0.5f, cw = W / PrismProcessor::maxSteps;
    g.setColour (line); g.drawHorizontalLine ((int) cy, x0, x0 + W);
    for (int i = 0; i < PrismProcessor::maxSteps; ++i)
    {
        const bool on = i < steps;
        auto bx = x0 + i * cw + 1.f;
        g.setColour (on ? ((i / 4) % 2 == 0 ? panelHi : panel.brighter (0.02f)) : bg);
        g.fillRect (bx, y0, cw - 2.f, H);
        if (on)
        {
            const float v = proc.getStep (i), bh = std::abs (v) * (H * 0.48f);
            g.setColour (v >= 0 ? red : redDark.brighter (0.2f));
            g.fillRect (bx, v >= 0 ? cy - bh : cy, cw - 2.f, std::max (2.f, bh));
            if (i == cur) { g.setColour (juce::Colours::white.withAlpha (0.65f)); g.drawRect (bx, y0, cw - 2.f, H, 1.2f); }
        }
    }
}

void SeqGrid::setAt (const juce::MouseEvent& e)
{
    const float x0 = 8.f, W = (float) getWidth() - 16.f, y0 = 22.f, H = (float) getHeight() - 28.f;
    const int i = (int) (((float) e.x - x0) / (W / PrismProcessor::maxSteps));
    if (i < 0 || i >= PrismProcessor::maxSteps) return;
    const float v = e.mods.isPopupMenu() ? 0.f : juce::jlimit (-1.f, 1.f, 1.f - 2.f * ((float) e.y - y0) / H);
    proc.setStep (i, v); repaint();
}

// =============================================================== Editeur Prism
Panel* PrismEditor::addPanel (const juce::String& title, const juce::String& power)
{
    panels.push_back (std::make_unique<Panel> (proc.apvts, title, power));
    addAndMakeVisible (*panels.back()); return panels.back().get();
}

PrismEditor::PrismEditor (PrismProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      presetBar (p.apvts, p.presets, "PrismFilter", { "vol" }),
      master (p.apvts, "vol", "MASTER"),
      wave (p), filterDisp (p), seqGrid (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    for (auto* c : { (juce::Component*) &presetBar, (juce::Component*) &master, (juce::Component*) &loadBtn,
                     (juce::Component*) &shapeBtn, (juce::Component*) &wave, (juce::Component*) &filterDisp, (juce::Component*) &seqGrid })
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

    auto* smp = addPanel ("LECTEUR DE SAMPLE");
    smp->addCombo ("src", "SOURCE"); smp->addCombo ("trig", "TRIG"); smp->addCombo ("loopMode", "BOUCLE");
    smp->addToggle ("rev", "REV"); smp->addKnob ("start", "START"); smp->addKnob ("end", "END");
    smp->addKnob ("speed", "SPEED"); smp->addKnob ("root", "ROOT"); smp->addKnob ("semi", "SEMI");
    smp->addKnob ("fine", "FINE"); smp->addKnob ("glide", "GLIDE");

    auto* grn = addPanel ("GRANULAIRE", "grnOn");
    grn->addKnob ("grnSize", "TAILLE"); grn->addKnob ("grnDens", "DENSITE");
    grn->addKnob ("grnJit", "JITTER"); grn->addKnob ("grnSpread", "STEREO"); grn->addKnob ("grnPitch", "PITCH");

    auto* f1 = addPanel ("FILTRE 1", "f1On");
    f1->addCombo ("f1Type", "TYPE"); f1->addKnob ("f1Cut", "CUTOFF"); f1->addKnob ("f1Res", "RESO");
    f1->addKnob ("f1Drive", "DRIVE"); f1->addKnob ("f1Morph", "VOYELLE"); f1->addKnob ("f1Key", "KEY"); f1->addKnob ("f1Pan", "PAN");

    auto* f2 = addPanel ("FILTRE 2", "f2On");
    f2->addCombo ("f2Type", "TYPE"); f2->addKnob ("f2Cut", "CUTOFF"); f2->addKnob ("f2Res", "RESO");
    f2->addKnob ("f2Drive", "DRIVE"); f2->addKnob ("f2Morph", "VOYELLE"); f2->addKnob ("f2Key", "KEY"); f2->addKnob ("f2Pan", "PAN");

    auto* rt = addPanel ("ROUTAGE");
    rt->addCombo ("routing", "MODE"); rt->addKnob ("fMix", "BALANCE");

    auto* env = addPanel ("ENVELOPPES (AMP & FILTRE)");
    env->addKnob ("aAtk", "AMP A"); env->addKnob ("aDec", "AMP D"); env->addKnob ("aSus", "AMP S"); env->addKnob ("aRel", "AMP R");
    env->addKnob ("fAtk", "FLT A"); env->addKnob ("fDec", "FLT D"); env->addKnob ("fSus", "FLT S"); env->addKnob ("fRel", "FLT R");
    env->addKnob ("fe1", "ENV>F1"); env->addKnob ("fe2", "ENV>F2"); env->addKnob ("velMod", "VEL");

    for (int i = 1; i <= 2; ++i)
    {
        auto p = "l" + juce::String (i);
        auto* l = addPanel ("LFO " + juce::String (i));
        l->addCombo (p + "Shape", "FORME"); l->addToggle (p + "Sync", "SYNC"); l->addCombo (p + "Div", "DIV");
        l->addKnob (p + "Rate", "HZ"); l->addKnob (p + "C1", ">F1"); l->addKnob (p + "C2", ">F2");
        l->addKnob (p + "Res", ">RES"); l->addKnob (p + "Pan", ">PAN"); l->addKnob (p + "Pitch", ">PITCH");
    }

    auto* sq = addPanel ("SEQUENCEUR DE FILTRE", "seqOn");
    sq->addKnob ("seqSteps", "PAS"); sq->addCombo ("seqDiv", "DIV"); sq->addKnob ("seqSmooth", "SMOOTH");
    sq->addKnob ("seqSwing", "SWING"); sq->addKnob ("seqC1", ">F1"); sq->addKnob ("seqC2", ">F2");
    sq->addKnob ("seqAmp", ">GATE"); sq->addKnob ("seqPitch", ">PITCH");

    auto* fx = addPanel ("CRUSH / RING / PHASER / DELAY / REVERB");
    fx->addKnob ("crush", "CRUSH"); fx->addKnob ("ring", "RING"); fx->addKnob ("ringHz", "R.HZ");
    fx->addKnob ("phRate", "PH.VIT"); fx->addKnob ("phMix", "PH.MIX");
    fx->addCombo ("dlyDiv", "DLY DIV"); fx->addKnob ("dlyFb", "DLY FB"); fx->addKnob ("dlyWob", "WOBBLE");
    fx->addKnob ("dlySmear", "SMEAR"); fx->addKnob ("dlyMix", "DLY MIX");
    fx->addKnob ("rvSize", "RV SIZE"); fx->addKnob ("rvMix", "RV MIX");

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, redDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, red);
    keyboard.setAvailableRange (36, 84); keyboard.setKeyWidth (18.f);
    addAndMakeVisible (keyboard);
    presetBar.applyFactory (0);
    setSize (1280, 740);
    startTimerHz (30);
}

void PrismEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (28.0f).withStyle ("Bold")));
    g.drawText ("PRISM", 16, 6, 200, 32, juce::Justification::left);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("PSYCHEDELIC FILTER SAMPLER", 18, 36, 260, 14, juce::Justification::left);
    g.setColour (line); g.drawHorizontalLine (58, 12.0f, (float) getWidth() - 12);
}

void PrismEditor::resized()
{
    presetBar.setBounds (270, 16, 420, 28);
    loadBtn.setBounds (710, 16, 110, 28);
    shapeBtn.setBounds (828, 16, 120, 28);
    master.setBounds (getWidth() - 78, 2, 58, 56);

    const int x = 12, y0 = 64, vh = 96;
    wave.setBounds (x, y0, 410, vh);
    filterDisp.setBounds (x + 418, y0, 330, vh);
    seqGrid.setBounds (x + 756, y0, getWidth() - x * 2 - 756, vh);

    const int py = y0 + vh + 6, dy = Panel::height + 6;
    layoutRow (x, py,          { panels[0].get(), panels[1].get() });
    layoutRow (x, py + dy,     { panels[2].get(), panels[3].get(), panels[4].get() });
    layoutRow (x, py + dy * 2, { panels[5].get(), panels[8].get() });
    layoutRow (x, py + dy * 3, { panels[6].get(), panels[7].get() });
    layoutRow (x, py + dy * 4, { panels[9].get() });
    keyboard.setBounds (x + panels[9]->preferredWidth() + 8, py + dy * 4, getWidth() - x * 2 - panels[9]->preferredWidth() - 8, Panel::height);
}
