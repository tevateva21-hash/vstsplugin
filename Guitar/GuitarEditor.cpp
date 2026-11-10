#include "GuitarEditor.h"
using namespace PsyUI;

Panel* GuitarEditor::addPanel (const juce::String& title, const juce::String& power)
{
    panels.push_back (std::make_unique<Panel> (proc.apvts, title, power));
    addAndMakeVisible (*panels.back()); return panels.back().get();
}

GuitarEditor::GuitarEditor (GuitarProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      presetBar (p.apvts, p.presets, "HeliosGuitar", { "vol" }),
      master (p.apvts, "vol", "MASTER"),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (presetBar); addAndMakeVisible (master);

    auto* str = addPanel ("CORDES / MEDIATOR");
    str->addKnob ("pickPos", "PICK POS"); str->addKnob ("pickHard", "PICK HARD"); str->addKnob ("pickNoise", "BRUIT");
    str->addKnob ("body", "CORPS"); str->addKnob ("bright", "BRILLANCE"); str->addKnob ("sustain", "SUSTAIN");
    str->addKnob ("sustainPlus", "INFINI"); str->addKnob ("twelve", "12 CORDES"); str->addKnob ("spread", "STEREO");

    auto* play = addPanel ("JEU / ACCORDS / VIBRATO");
    play->addCombo ("mode", "MODE"); play->addCombo ("chord", "ACCORD"); play->addCombo ("strumDir", "SENS");
    play->addKnob ("strumMs", "STRUM"); play->addKnob ("glide", "GLIDE"); play->addKnob ("bendRange", "BEND");
    play->addKnob ("vibRate", "VIB VIT"); play->addKnob ("vibDepth", "VIB PROF");
    play->addKnob ("transpose", "TRANSP"); play->addKnob ("fine", "FINE");

    auto* cmp = addPanel ("COMPRESSEUR", "compOn");
    cmp->addKnob ("compThr", "SEUIL"); cmp->addKnob ("compRatio", "RATIO");
    cmp->addKnob ("compAtk", "ATK"); cmp->addKnob ("compRel", "REL"); cmp->addKnob ("compMakeup", "GAIN");

    auto* oct = addPanel ("OCTAVER", "octOn");
    oct->addKnob ("octDown", "OCT -1"); oct->addKnob ("octUp", "OCT +1");

    auto* drv = addPanel ("FUZZ / OVERDRIVE / MUFF", "drvOn");
    drv->addCombo ("drvType", "TYPE"); drv->addKnob ("drvGain", "GAIN");
    drv->addKnob ("drvTone", "TONE"); drv->addKnob ("drvLevel", "LEVEL"); drv->addKnob ("drvMix", "MIX");

    auto* wah = addPanel ("WAH", "wahOn");
    wah->addCombo ("wahMode", "MODE"); wah->addKnob ("wahPedal", "PEDALE");
    wah->addKnob ("wahSens", "SENS"); wah->addKnob ("wahRate", "VIT"); wah->addKnob ("wahQ", "RESO"); wah->addKnob ("wahMix", "MIX");

    auto* amp = addPanel ("AMPLI & BAFFLE", "ampOn");
    amp->addKnob ("ampGain", "DRIVE"); amp->addKnob ("bass", "BASS"); amp->addKnob ("mid", "MID");
    amp->addKnob ("treble", "TREBLE"); amp->addKnob ("presence", "PRES"); amp->addCombo ("cab", "CAB");

    auto* trm = addPanel ("TREMOLO", "tremOn");
    trm->addCombo ("tremShape", "FORME"); trm->addKnob ("tremRate", "VIT");
    trm->addKnob ("tremDepth", "PROF"); trm->addKnob ("tremSpread", "STEREO");

    auto* ph = addPanel ("PHASER", "phOn");
    ph->addKnob ("phRate", "VIT"); ph->addKnob ("phDepth", "PROF");
    ph->addKnob ("phCentre", "CENTRE"); ph->addKnob ("phFb", "FB"); ph->addKnob ("phMix", "MIX");

    auto* mod = addPanel ("CHORUS / FLANGER / LESLIE");
    mod->addCombo ("modType", "TYPE"); mod->addKnob ("modRate", "VIT"); mod->addKnob ("modDepth", "PROF");
    mod->addKnob ("modFb", "FB"); mod->addKnob ("modMix", "MIX"); mod->addToggle ("lesFast", "FAST");

    auto* dly = addPanel ("TAPE / BINSON DELAY", "dlyOn");
    dly->addToggle ("dlySync", "SYNC"); dly->addCombo ("dlyDiv", "DIV"); dly->addKnob ("dlyMs", "MS");
    dly->addKnob ("dlyFb", "FB"); dly->addKnob ("dlyTone", "TONE"); dly->addKnob ("dlyWob", "WOBBLE");
    dly->addKnob ("dlyWobRate", "W.VIT"); dly->addKnob ("dlySpread", "PINGPONG"); dly->addKnob ("dlySmear", "SMEAR");
    dly->addKnob ("dlyDrive", "DRIVE"); dly->addKnob ("dlyMix", "MIX");

    auto* rv = addPanel ("REVERB (SPRING / SHIMMER)", "rvOn");
    rv->addCombo ("rvType", "TYPE"); rv->addKnob ("rvSize", "TAILLE");
    rv->addKnob ("rvDamp", "DAMP"); rv->addKnob ("rvShimmer", "SHIMMER"); rv->addKnob ("rvMix", "MIX");

    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, panelHi);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, bg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, line);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, redDark);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, red);
    keyboard.setAvailableRange (36, 96); keyboard.setKeyWidth (18.0f);
    addAndMakeVisible (keyboard);
    presetBar.applyFactory (0);
    setSize (1260, 620);
}

void GuitarEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (28.0f).withStyle ("Bold")));
    g.drawText ("HELIOS", 16, 6, 200, 32, juce::Justification::left);
    g.setColour (textDim); g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("PSYCHEDELIC GUITAR SYNTHESIZER", 18, 36, 260, 14, juce::Justification::left);
    g.setColour (line); g.drawHorizontalLine (58, 12.0f, (float) getWidth() - 12);
}

void GuitarEditor::resized()
{
    presetBar.setBounds (290, 16, 420, 28);
    master.setBounds (getWidth() - 78, 2, 58, 56);
    const int x = 12, y0 = 66, dy = Panel::height + 6;
    layoutRow (x, y0,          { panels[0].get(), panels[1].get() });
    layoutRow (x, y0 + dy,     { panels[2].get(), panels[3].get(), panels[4].get(), panels[5].get() });
    layoutRow (x, y0 + dy * 2, { panels[6].get(), panels[7].get(), panels[8].get() });
    layoutRow (x, y0 + dy * 3, { panels[9].get(), panels[10].get(), panels[11].get() });
    keyboard.setBounds (12, y0 + dy * 4 + 2, getWidth() - 24, 62);
}
