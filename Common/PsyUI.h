#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Charte graphique commune : noir et rouge, sobre, mat et lisible.
namespace PsyUI
{
const juce::Colour bg       (0xff0b0b0d);
const juce::Colour panel    (0xff141418);
const juce::Colour panelHi  (0xff1e1e24);
const juce::Colour line     (0xff2c2c34);
const juce::Colour red      (0xffcc1824);
const juce::Colour redDark  (0xff660d13);
const juce::Colour redHot   (0xffff3340);
const juce::Colour text     (0xffdedee4);
const juce::Colour textDim  (0xff8c8c96);
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
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")); }
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

// Barre d'onglets pour basculer entre differentes vues (ex: Synthese / Modulation / Effets).
class TabBar : public juce::Component
{
public:
    TabBar (std::initializer_list<const char*> names, std::function<void (int)> onSelect);
    void select (int idx);
    int current() const { return active; }
    void resized() override;
private:
    std::vector<std::unique_ptr<juce::TextButton>> btns;
    std::function<void (int)> cb;
    int active = 0;
};

// Panneau de commandes (titre avec bandeau, bouton on/off optionnel, potards / menus / interrupteurs).
class Panel : public juce::Component
{
public:
    Panel (juce::AudioProcessorValueTreeState&, const juce::String& title, const juce::String& powerId = {});
    void addKnob (const juce::String& id, const juce::String& label, int width = 66);
    void addCombo (const juce::String& id, const juce::String& label, int width = 114);
    void addToggle (const juce::String& id, const juce::String& label, int width = 68);
    int preferredWidth() const;
    void paint (juce::Graphics&) override;
    void resized() override;
    static constexpr int height = 122;
private:
    struct Cell { std::unique_ptr<juce::Component> comp; std::unique_ptr<juce::Label> label; int w; };
    juce::AudioProcessorValueTreeState& apvts;
    juce::String title;
    std::vector<Cell> cells;
    std::vector<std::shared_ptr<void>> att;
    juce::TextButton power { "ON" };
    bool hasPower = false;
};

// Place des panneaux sur une ligne ; si totalWidth > 0, etire proportionnellement pour remplir toute la largeur.
int layoutRow (int x, int y, const std::vector<Panel*>& row, int totalWidth = 0, int rowHeight = Panel::height);

