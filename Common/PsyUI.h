#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Charte graphique commune : noir et rouge, sobre et mat.
namespace PsyUI
{
const juce::Colour bg       (0xff09090a);
const juce::Colour panel    (0xff131315);
const juce::Colour panelHi  (0xff1b1b1e);
const juce::Colour line     (0xff2a2a2e);
const juce::Colour red      (0xffb3121b);
const juce::Colour redDark  (0xff5c0b10);
const juce::Colour redHot   (0xffe0262f);
const juce::Colour text     (0xffc8c8c8);
const juce::Colour textDim  (0xff75757a);
}

class PsyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PsyLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (12.0f)); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (12.0f)); }
};

class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name, bool withLabel = true);
    void resized() override;
    juce::Slider slider;
private:
    juce::Label label;
    bool hasLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Panneau de commandes (titre, bouton on/off optionnel, potards / menus / interrupteurs).
class Panel : public juce::Component
{
public:
    Panel (juce::AudioProcessorValueTreeState&, const juce::String& title, const juce::String& powerId = {});
    void addKnob (const juce::String& id, const juce::String& label);
    void addCombo (const juce::String& id, const juce::String& label);
    void addToggle (const juce::String& id, const juce::String& label);
    int preferredWidth() const;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int height = 108;
private:
    struct Cell { std::unique_ptr<juce::Component> comp; std::unique_ptr<juce::Label> label; int w; };
    juce::AudioProcessorValueTreeState& apvts;
    juce::String title;
    std::vector<Cell> cells;
    std::vector<std::shared_ptr<void>> att;
    juce::TextButton power { "ON" };
    bool hasPower = false;
};

// Place des panneaux sur une ligne ; renvoie la largeur utilisee.
int layoutRow (int x, int y, const std::vector<Panel*>& row);
