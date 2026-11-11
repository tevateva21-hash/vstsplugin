#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include "PsyDelay.h"
#include "PresetBar.h"

// HELIOS GUITAR : guitare synthetisee (Karplus-Strong polyphonique) + pedalier psychedelique.
class GuitarProcessor : public juce::AudioProcessor
{
public:
    GuitarProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override
    {
        const auto& out = l.getMainOutputChannelSet();
        return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
    }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Helios Guitar"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& d) override { if (auto x = apvts.copyState().createXml()) copyXmlToBinary (*x, d); }
    void setStateInformation (const void* d, int s) override
    { if (auto x = getXmlFromBinary (d, s)) if (x->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*x)); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::vector<FactoryPreset> presets;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float P (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    float noteFreq (int note) const;
    void noteOn (int key, float vel);
    void noteOff (int key);
    void startNote (int note, float vel, int owner, int delaySamples, float gain);
    void pluck (int slot, int note, float vel, int owner, float gain);
    int  allocVoice();
    void updateTone();
    float shape (float p, int type) const;

    struct Voice
    {
        bool active = false, released = false; int owner = -1, note = 0, age = 0;
        float vel = 0, freq = 110, target = 110, lp = 0, env = 0, g = 0.99f, gain = 1, click = 0, panL = 0.7f, panR = 0.7f;
        std::vector<float> buf; int w = 0;
    };
    struct Pending { bool active = false; int delay = 0, note = 0, owner = 0; float vel = 0, gain = 1; };

    static constexpr int numVoices = 12, numPending = 32;
    Voice voices[numVoices]; Pending pend[numPending];
    std::vector<int> held; int bufSize = 4096, monoVoice = 0; bool strumFlip = false;
    double sr = 44100;
    float bend = 0, modWheel = 0, vibPh = 0, glideCoef = 1, damp = 0.5f;
    juce::Random rng { 99 };

    // effets
    struct Ch
    {
        Psy::OnePole dlp1, dlp2, dlpT, wasteLp, tremLp; Psy::SVFT wah; Psy::Biquad bass, mid, treb, pres, cabHp, cabLp1, cabLp2, cabPk;
        float compEnvDummy = 0;
    };
    Ch ch[2];
    float compEnv = 0, wahEnv = 0, tremPh = 0, wahPh = 0, lastCompMakeup = 0;
    Psy::PitchShifter psDown, psUp, psShim[2];
    Psy::ModDelay modDelay; Psy::Leslie leslie; Psy::SpringReverb spring;
    Eclipse::PsyDelay delay; double delayTime = 10000;
    Psy::Biquad body[3];
    juce::dsp::Phaser<float> phaser;
    juce::Reverb reverb;
    juce::AudioBuffer<float> rvBuf; std::vector<float> shimL, shimR;
    float lastBass = -99, lastMid = -99, lastTreb = -99, lastPres = -99; int lastCab = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GuitarProcessor)
};
