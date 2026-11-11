#pragma once
#include "PluginProcessor.h"
#include "PsyUI.h"
using namespace PsyUI;

struct Brush { std::atomic<int> fx { 0 }; std::atomic<int> vel { 100 }; std::atomic<int> laneFx { 1 }; };

class StepGrid : public juce::Component
{
public:
    StepGrid (EclipseProcessor&, int track, Brush&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    int stepAt (juce::Point<int>) const;
    void apply (const juce::MouseEvent&, bool first);
    EclipseProcessor& proc; int track; Brush& brush;
    bool erasing = false; int lastStep = -1;
};

class LaneGrid : public juce::Component
{
public:
    LaneGrid (EclipseProcessor&, Brush&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
private:
    int stepAt (juce::Point<int>) const;
    void apply (const juce::MouseEvent&, bool first);
    EclipseProcessor& proc; Brush& brush;
    bool erasing = false; int lastStep = -1;
};

class TrackRow : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    TrackRow (EclipseProcessor&, int track, Brush&);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void refreshTooltip();
    static constexpr int rowH = 52, cell = 17;
private:
    void showMenu();
    void chooseFile();
    std::unique_ptr<juce::FileChooser> chooser;
    EclipseProcessor& proc; int track;
    juce::TextButton padBtn, mute { "M" }, menu { "..." }, load { "LOAD" };
    juce::ComboBox sample, rate;
    juce::Slider steps;
    StepGrid grid;
    std::unique_ptr<Knob> knobs[6];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aSample, aRate;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aMute;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aSteps;
};

class EclipseEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit EclipseEditor (EclipseProcessor&);
    ~EclipseEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void setTab (int idx);
    void panelBox (juce::Graphics&, juce::Rectangle<int>, const juce::String&);

    EclipseProcessor& proc;
    PsyLookAndFeel lnf;
    Brush brush;
    TabBar tabs;
    std::vector<std::unique_ptr<TrackRow>> rows;
    std::vector<std::unique_ptr<Panel>> fxPanels;
    juce::TextButton play { "PLAY / RUN" }, exportBtn { "EXPORT MIDI" }, clearBtn { "CLEAR" };
    juce::TooltipWindow tooltip { this, 400 };
    LaneGrid laneGrid;
    juce::Slider laneSteps; juce::TextButton laneMenu { "..." };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aLaneSteps;
    juce::ComboBox laneBrush;
    juce::ComboBox presets, keyTrack, brushFx;
    juce::Slider brushVel;
    juce::Label brushLabel, keyLabel;
    std::unique_ptr<Knob> bpm, swing, master;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aPlay;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aKey;
    juce::MidiKeyboardComponent keyboard;
    std::unique_ptr<juce::FileChooser> chooser;
    int laneY = 0, laneH = 44;
    juce::Rectangle<int> pBrush;
};

