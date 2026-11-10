#pragma once
#include "GuitarProcessor.h"
#include "PsyUI.h"

class GuitarEditor : public juce::AudioProcessorEditor
{
public:
    explicit GuitarEditor (GuitarProcessor&);
    ~GuitarEditor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    Panel* addPanel (const juce::String& title, const juce::String& power = {});
    GuitarProcessor& proc;
    PsyLookAndFeel lnf;
    juce::TooltipWindow tooltip { this, 400 };
    PresetBar presetBar;
    Knob master;
    std::vector<std::unique_ptr<Panel>> panels;
    juce::MidiKeyboardComponent keyboard;
};
