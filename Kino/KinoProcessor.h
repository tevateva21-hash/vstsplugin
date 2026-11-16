#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

struct KinoStep
{
    bool active = false;
    int degree = 0;       // Scale degree offset (-7 .. +14)
    int velocity = 100;   // 1 .. 127
    float gate = 0.65f;   // 0.05 .. 1.0
    int ratchet = 1;      // 1 .. 4
    bool slide = false;
    bool accent = false;
    int prob = 100;       // 0 .. 100 %
    int octJump = 0;      // -1, 0, +1
};

struct KinoModel
{
    juce::String category;
    juce::String name;
    int root = 6;         // F#
    int octave = 2;
    int scale = 0;        // Phrygian Dominant
    int steps = 16;
    int rate = 2;         // 0:1/4, 1:1/8, 2:1/16, 3:1/32, 4:1/8T, 5:1/16T
    int intCurve = 0;     // 0:Flat, 1:Crescendo, 2:Decrescendo, 3:Pulse, 4:Random
    int chordMode = 0;    // 0:Single, 1:Oct, 2:Fifth, 3:Minor, 4:Sus4, 5:7th
    float swing = 0.0f;
    std::vector<KinoStep> pattern;
};

class KinoProcessor : public juce::AudioProcessor
{
public:
    static constexpr int maxSteps = 32;

    KinoProcessor();
    ~KinoProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Kino MIDI"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Step access (thread-safe via SpinLock)
    KinoStep getStep (int idx) const;
    void setStep (int idx, const KinoStep& s);
    void toggleCell (int step, int degree);

    // Musical helpers
    int degreeToMidiNote (int degree, int octJump) const;
    juce::String noteNameForDegree (int degree) const;
    float intensityMultiplierForStep (int step, int totalSteps) const;

    // Generators & Models
    const std::vector<KinoModel>& getModels() const { return models; }
    void loadModel (int index);
    void applyEuclidean (int pulses, int rotation);
    void mutatePattern();
    void clearPattern();

    // MIDI File Export
    bool exportMidiFile (const juce::File& file) const;
    juce::File createTempMidiFile() const;

    float P (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    std::atomic<int> currentStep { 0 };
    std::atomic<int> currentModelIndex { 0 };
    std::atomic<float> visualPulse { 0.0f }; // Used to animate the 3D Spectators!

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void initModels();

    mutable juce::SpinLock stepLock;
    std::array<KinoStep, maxSteps> steps;
    std::vector<KinoModel> models;

    // Sequencer timing & active note tracking
    double sr = 44100.0;
    double stepPhase = 0.0;
    int seqStep = 0;
    int lastRatchetIdx = -1;
    int liveMidiTranspose = 0;
    bool hasLiveTranspose = false;
    juce::Random rng;

    struct PendingNoteOff
    {
        int note = 0;
        int channel = 1;
        double samplesRemaining = 0.0;
    };
    std::vector<PendingNoteOff> pendingOffs;

    // Basic Saw Wave internal preview synth (8 voices)
    struct SawVoice
    {
        bool active = false;
        int note = 60;
        double phase = 0.0;
        double freq = 261.63;
        double targetFreq = 261.63;
        float env = 0.0f;
        float vel = 0.8f;
        float lpState = 0.0f;
        bool accent = false;
    };
    std::array<SawVoice, 8> voices;

    void triggerNoteOn (juce::MidiBuffer& out, int sampleOffset, int note, int vel, int ch, double durSamples, bool slide, bool accent);
    void processInternalSaw (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KinoProcessor)
};
