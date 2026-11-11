#include "GuitarEditor.h"
using namespace PsyUI;

Panel* GuitarEditor::addPanel (const juce::String& title, const juce::String& power)
{
    panels.push_back (std::make_unique<Panel> (proc.apvts, title, power));
    addChildComponent (*panels.back()); return panels.back().get();
}

GuitarEditor::GuitarEditor (GuitarProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      presetBar (p.apvts, p.presets, "HeliosGuitar", { "vol" }),
      master (p.apvts, "vol", "MASTER"),
      tabs ({ "1. GUITARE, PEDALES & AMPLI", "2. MODULATIONS, DELAY & REVERB" }, [this] (int idx) { setTab (idx); }),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (presetBar); addAndMakeVisible (master); addAndMakeVisible (tabs);

    // --- FENETRE 1 : CORDES, JEU, COMP, OCTAVER, DRIVE, WAH, AMPLI (panels 0..6) ---
    auto* str = addPanel ("CORDES / MEDIATOR / CAISSE");
    str->addKnob ("pickPos", "PICK POS"); str->addKnob ("pickHard", "PICK HARD"); str->addKnob ("pickNoise", "BRUIT");
    str->addKnob ("body", "CORPS"); str->addKnob ("bright", "BRILLANCE"); str->addKnob ("sustain", "SUSTAIN");
    str->addKnob ("sustainPlus", "INFINI"); str->addKnob ("twelve", "12 CORDES"); str->addKnob ("spread", "STEREO");

    auto* play = addPanel ("JEU / ACCORDS / VIBRATO");
    play->addCombo ("mode", "MODE", 110); play->addCombo ("chord", "ACCORD", 114); play->addCombo ("strumDir", "SENS", 92);
    play->addKnob ("strumMs", "STRUM"); play->addKnob ("glide", "GLIDE"); play->addKnob ("bendRange", "BEND");
    play->addKnob ("vibRate", "VIB VIT"); play->addKnob ("vibDepth", "VIB PROF");
    play->addKnob ("transpose", "TRANSP"); play->addKnob ("fine", "FINE");

    auto* cmp = addPanel ("COMPRESSEUR", "compOn");
    cmp->addKnob ("compThr", "SEUIL"); cmp->addKnob ("compRatio", "RATIO");
    cmp->addKnob ("compAtk", "ATK"); cmp->addKnob ("compRel", "REL"); cmp->addKnob ("compMakeup", "GAIN");

    auto* oct = addPanel ("OCTAVER", "octOn");
    oct->addKnob ("octDown", "OCT -1"); oct->addKnob ("octUp", "OCT +1");

    auto* drv = addPanel ("FUZZ / OVERDRIVE / BIG MUFF", "drvOn");
    drv->addCombo ("drvType", "TYPE", 110); drv->addKnob ("drvGain", "GAIN");
    drv->addKnob ("drvTone", "TONE"); drv->addKnob ("drvLevel", "LEVEL"); drv->addKnob ("drvMix", "MIX");

    auto* wah = addPanel ("PEDALE WAH-WAH", "wahOn");
    wah->addCombo ("wahMode", "MODE", 98); wah->addKnob ("wahPedal", "PEDALE");
    wah->addKnob ("wahSens", "SENS"); wah->addKnob ("wahRate", "VIT"); wah->addKnob ("wahQ", "RESO"); wah->addKnob ("wahMix", "MIX");

    auto* amp = addPanel ("AMPLI HIWATT / PLEXI & BAFFLE", "ampOn");
    amp->addKnob ("ampGain", "DRIVE"); amp->addKnob ("bass", "BASS"); amp->addKnob ("mid", "MID");
    amp->addKnob ("treble", "TREBLE"); amp->addKnob ("presence", "PRES"); amp->addCombo ("cab", "CABINET", 96);

    // --- FENETRE 2 : TREMOLO, PHASER, CHORUS/LESLIE, TAPE DELAY, SHIMMER REVERB (panels 7..11) ---
    auto* trm = addPanel ("TREMOLO OPTIQUE / HARMONIQUE", "tremOn");
    trm->addCombo ("tremShape", "FORME", 110); trm->addKnob ("tremRate", "VITESSE");
    trm->addKnob ("tremDepth", "PROFONDEUR"); trm->addKnob ("tremSpread", "STEREO");

    auto* ph = addPanel ("PHASER PSYCHEDELIQUE", "phOn");
    ph->addKnob ("phRate", "VITESSE"); ph->addKnob ("phDepth", "PROFONDEUR");
    ph->addKnob ("phCentre", "CENTRE"); ph->addKnob ("phFb", "FEEDBACK"); ph->addKnob ("phMix", "MIX");

    auto* mod = addPanel ("CHORUS / FLANGER / CABINE LESLIE");
    mod->addCombo ("modType", "MODE", 110); mod->addKnob ("modRate", "VITESSE"); mod->addKnob ("modDepth", "PROFONDEUR");
    mod->addKnob ("modFb", "FEEDBACK"); mod->addKnob ("modMix", "MIX"); mod->addToggle ("lesFast", "LESLIE FAST", 90);

    auto* dly = addPanel ("TAPE / BINSON ECHOREC DELAY", "dlyOn");
    dly->addToggle ("dlySync", "SYNC", 64); dly->addCombo ("dlyDiv", "DIV", 86); dly->addKnob ("dlyMs", "TEMPS MS");
    dly->addKnob ("dlyFb", "FEEDBACK"); dly->addKnob ("dlyTone", "TONE"); dly->addKnob ("dlyWob", "WOBBLE");
    dly->addKnob ("dlyWobRate", "W.VIT"); dly->addKnob ("dlySpread", "PINGPONG"); dly->addKnob ("dlySmear", "SMEAR");
    dly->addKnob ("dlyDrive", "DRIVE"); dly->addKnob ("dlyMix", "MIX");

    auto* rv = addPanel ("REVERB (SPRING / HALL / SHIMMER)", "rvOn");
    rv->addCombo ("rvType", "TYPE", 104); rv->addKnob ("rvSize", "TAILLE");
    rv->addKnob ("rvDamp", "DAMPING"); rv->addKnob ("rvShimmer", "SHIMMER"); rv->addKnob ("rvMix", "MIX");

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, redDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, red);
    keyboard.setAvailableRange (36, 96); keyboard.setKeyWidth (20.0f);
    addAndMakeVisible (keyboard);
    presetBar.applyFactory (0);
    setSize (1260, 650);
    setTab (0);
}

void GuitarEditor::setTab (int idx)
{
    for (size_t i = 0; i < panels.size(); ++i)
        panels[i]->setVisible ((idx == 0 && i <= 6) || (idx == 1 && i >= 7));
    resized();
    repaint();
}

void GuitarEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (redHot); g.setFont (juce::Font (juce::FontOptions (28.0f).withStyle ("Bold")));
    g.drawText ("HELIOS", 16, 6, 200, 32, juce::Justification::left);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("PSYCHEDELIC GUITAR SYNTHESIZER", 18, 36, 260, 14, juce::Justification::left);
    g.setColour (line); g.drawHorizontalLine (60, 14.0f, (float) getWidth() - 14);
}

void GuitarEditor::resized()
{
    const int margin = 14;
    const int totalW = getWidth() - margin * 2;
    presetBar.setBounds (290, 14, 430, 30);
    master.setBounds (getWidth() - 78, 2, 60, 56);
    tabs.setBounds (margin, 68, totalW, 34);

    const int y0 = 112;
    if (tabs.current() == 0)
    {
        const int rowH = 142;
        layoutRow (margin, y0,                    { panels[0].get(), panels[1].get() }, totalW, rowH);
        layoutRow (margin, y0 + rowH + 10,        { panels[2].get(), panels[3].get(), panels[4].get() }, totalW, rowH);
        layoutRow (margin, y0 + (rowH + 10) * 2,  { panels[5].get(), panels[6].get() }, totalW, rowH);
    }
    else
    {
        const int rowH = 215;
        layoutRow (margin, y0,             { panels[7].get(), panels[8].get(), panels[9].get() }, totalW, rowH);
        layoutRow (margin, y0 + rowH + 12, { panels[10].get(), panels[11].get() }, totalW, rowH);
    }
    keyboard.setBounds (margin, getHeight() - 74, totalW, 64);
}

