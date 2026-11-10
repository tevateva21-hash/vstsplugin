#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "Dsp.h"
#include "PsyDelay.h"
#include "PresetBar.h"

// PRISM FILTER : sampler filtrant monophonique a 2 filtres, sequenceur de filtre, granulaire et effets.
class PrismProcessor : public juce::AudioProcessor
{
public:
    static constexpr int maxSteps = 32;
    PrismProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Prism Filter"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // sample
    bool loadUserSample (const juce::File&, bool select = true);
    const std::vector<float>* currentWave() const;
    juce::String getUserName() const { return userPath.isEmpty() ? juce::String() : juce::File (userPath).getFileName(); }
    float P (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    // sequenceur de filtre
    void setStep (int i, float v) { seq[i] = juce::jlimit (-1.f, 1.f, v); }
    float getStep (int i) const { return seq[i]; }
    int getSteps() const { return (int) P ("seqSteps"); }
    void setWave (int type); // 0 rampe +, 1 rampe -, 2 triangle, 3 sinus, 4 escalier, 5 trance, 6 aleatoire, 7 plat

    std::atomic<float> playRatio { 0 }, modCut1 { 1000 }, modCut2 { 2000 };
    std::atomic<int> curStep { -1 };
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::vector<FactoryPreset> presets;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void makeBank();

    // filtres : 0 LP12, 1 LP24, 2 HP12, 3 HP24, 4 BP12, 5 BP24, 6 Notch, 7 Ladder, 8 Comb+, 9 Comb-, 10 Formant
    struct MultiFilter
    {
        Psy::SVFT s1, s2, f[3]; float lad[4] {}; Psy::DelayLine comb;
        void init (double sr) { s1.reset(); s2.reset(); for (auto& x : f) x.reset(); for (auto& x : lad) x = 0; comb.init ((int) (sr * 0.05) + 16); }
        float process (float x, int type, float fc, float res, float drive, float morph, double sr);
    };
    struct ADSR
    {
        int st = 0; float v = 0;
        void noteOn (bool retrig) { if (retrig || st == 0) st = 1; }
        void noteOff() { if (st != 0) st = 4; }
        float tick (float a, float d, float s, float r, double sr);
    };
    struct Grain { bool on = false; double pos = 0, inc = 1; int age = 0, len = 1; float panL = 0.7f, panR = 0.7f; };

    std::vector<std::vector<float>> bank;
    std::atomic<const std::vector<float>*> userData { nullptr };
    std::vector<std::unique_ptr<std::vector<float>>> graveyard;
    juce::String userPath;
    juce::AudioFormatManager formats;

    std::atomic<float> seq[maxSteps];
    MultiFilter flt[2][2]; // [filtre 1/2][L/R]
    ADSR ampEnv, fltEnv;
    Psy::LFO lfo1, lfo2;
    Grain grains[16]; int grainCooldown = 0;
    std::vector<int> held;
    double sr = 44100, pos = 0, internalPpq = 0, delayTime = 10000;
    int dir = 1, holdCnt = 0; float curNote = 60, targetNote = 60, vel = 1, seqSmoothed = 0, heldL = 0, heldR = 0;
    bool gate = false;
    juce::Random rng { 17 };

    Eclipse::PsyDelay delay;
    juce::dsp::Phaser<float> phaser;
    juce::Reverb reverb;
    juce::AudioBuffer<float> rvBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PrismProcessor)
};
