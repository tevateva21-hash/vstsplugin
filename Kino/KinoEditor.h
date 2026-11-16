#pragma once
#include "KinoProcessor.h"
#include "KinoUI.h"
#include <juce_audio_utils/juce_audio_utils.h>

class DragMidiPill : public juce::Component
{
public:
    DragMidiPill (KinoProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent&) override {}
    void mouseDrag (const juce::MouseEvent& e) override;
private:
    KinoProcessor& proc;
    bool dragging = false;
};

class PianoRollGrid : public juce::Component
{
public:
    static constexpr int minDeg = -2;
    static constexpr int maxDeg = 11;
    static constexpr int numRows = maxDeg - minDeg + 1; // 14 scale degrees
    static constexpr int labelW = 48;

    PianoRollGrid (KinoProcessor& p);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    KinoProcessor& proc;
    int lastStep = -1, lastDeg = -99;
    bool erasing = false;
    void handlePointer (const juce::MouseEvent& e, bool first);
};

class GoofyStepStrip : public juce::Component
{
public:
    GoofyStepStrip (KinoProcessor& p);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    KinoProcessor& proc;
    bool draggingVel = false;
    void handleVelDrag (const juce::MouseEvent& e);
};

class KinoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit KinoEditor (KinoProcessor&);
    ~KinoEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawCard (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title);
    void showModelMenu();
    void updateModelButtonText();

    KinoProcessor& proc;
    KinoLookAndFeel lnf;

    // Top bar
    juce::TextButton playBtn { "PLAY" }, modelBtn, prevModelBtn { "<" }, nextModelBtn { ">" };
    juce::TextButton saveMidiBtn { "SAVE .MID" }, clearBtn { "CLEAR" };
    DragMidiPill dragMidi;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aPlay;
    std::unique_ptr<KinoKnob> kBpm, kSteps, kSwing;
    juce::ComboBox cRate;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aRate;

    // Main Piano Roll + Goofy Step Strip
    PianoRollGrid pianoRoll;
    GoofyStepStrip stepStrip;

    // Right cards controls
    juce::ComboBox cRoot, cScale, cChords, cIntCurve;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aRoot, aScale, aChords, aIntCurve;
    juce::TextButton liveTransBtn { "MIDI TRANSPOSE" }, euclidBtn { "EUCLID" }, mutateBtn { "MUTATE" }, sawOnBtn { "ON" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aLiveTrans, aSawOn;
    std::unique_ptr<KinoKnob> kOctave, kIntensity, kEucPulses, kEucRot, kSawVol, kSawCut, kSawDecay;

    // Goofy 3D Spectators
    std::vector<std::unique_ptr<SpectatorAvatar>> spectators;

    // Bottom MIDI keyboard
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;

    // Card bounds cached for paint()
    juce::Rectangle<int> cardSeq, cardScale, cardGen, cardSaw, cardCrew;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KinoEditor)
};
