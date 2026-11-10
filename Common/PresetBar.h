#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>

struct FactoryPreset
{
    juce::String category, name;
    std::vector<std::pair<juce::String, float>> values;   // id de parametre -> valeur reelle
};

// Barre de presets : menu par categories, suivant/precedent, sauvegarde utilisateur, de (mutation aleatoire).
class PresetBar : public juce::Component
{
public:
    PresetBar (juce::AudioProcessorValueTreeState&, const std::vector<FactoryPreset>&, const juce::String& pluginFolder,
               juce::StringArray excludedIds = {});
    void resized() override;
    void paint (juce::Graphics&) override;
    void applyFactory (int index);
private:
    void showMenu();
    void step (int dir);
    void saveUser();
    void loadUser (const juce::File&);
    void dice();
    juce::File userDir() const;
    juce::Array<juce::File> userFiles() const;
    void resetAll();

    juce::AudioProcessorValueTreeState& apvts;
    const std::vector<FactoryPreset>& presets;
    juce::String folder; juce::StringArray excluded;
    juce::TextButton menuBtn, prev { "<" }, next { ">" }, save { "SAVE" }, diceBtn { "DICE" };
    int current = -1;
    juce::Random rng;
};
