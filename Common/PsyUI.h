#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Charte graphique commune : Noir d'encre, Rouge Cinema, Blanc casse use, avec usage reduit de l'anthracite
namespace PsyUI
{
const juce::Colour bg         (0xff121113); // Noir d'encre profond (fond general aeré)
const juce::Colour panel      (0xff181717); // Noir d'encre sobre (evite les lourds blocs anthracites)
const juce::Colour panelHi    (0xff222020); // Subtile surbrillance pour bandeaux
const juce::Colour anthracite (0xff302e2e); // Anthracite restreint aux ombres / details secondaires
const juce::Colour grayEarth  (0xff65615f); // Gris moyen terreux (rails de potentiometres, vis)
const juce::Colour line       (0xff363433); // Lignes d'encrage fines
const juce::Colour red        (0xffff1b24); // Rouge vibrant "Cinema"
const juce::Colour redDark    (0xff7a0d13); // Rouge fonce (etat clique / fond actif sombre)
const juce::Colour redHot     (0xffff2e38); // Rouge ecarlate (LED active, indicateurs)
const juce::Colour text       (0xffede7e1); // Blanc casse use (grande clarte et lisibilite)
const juce::Colour textDim    (0xffa29c96); // Blanc casse attenue (sous-titres et reperes)
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

