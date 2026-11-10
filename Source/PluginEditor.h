#pragma once
#include "PluginProcessor.h"

namespace EclipseUI
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

class EclipseLookAndFeel : public juce::LookAndFeel_V4
{
public:
    EclipseLookAndFeel();
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
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
    void refreshTooltip();
    static constexpr int rowH = 44, cell = 16;
private:
    void showMenu();
    void chooseFile();
    std::unique_ptr<juce::FileChooser> chooser;
    EclipseProcessor& proc; int track;
    juce::Label num; juce::TextButton mute { "M" }, menu { "..." }, load { "LOAD" };
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
    void timerCallback() override { for (auto& r : rows) r->repaint(); laneGrid.repaint(); }
    void panelBox (juce::Graphics&, juce::Rectangle<int>, const juce::String&);

    EclipseProcessor& proc;
    EclipseLookAndFeel lnf;
    Brush brush;
    std::vector<std::unique_ptr<TrackRow>> rows;
    std::vector<std::unique_ptr<Knob>> fxKnobs;
    juce::TextButton play { "PLAY" }, exportBtn { "EXPORT MIDI" }, clearBtn { "CLEAR" };
    juce::TooltipWindow tooltip { this, 400 };
    LaneGrid laneGrid;
    juce::Slider laneSteps; juce::TextButton laneMenu { "..." };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> aLaneSteps;
    juce::ComboBox laneBrush;
    juce::ComboBox presets, keyTrack, delayTime, filterType, brushFx;
    juce::Slider brushVel;
    juce::Label brushLabel, keyLabel;
    std::unique_ptr<Knob> bpm, swing, master;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> aPlay;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> aKey, aDly, aFType;
    std::unique_ptr<juce::FileChooser> chooser;
    int laneY = 0, laneH = 40;
    juce::Rectangle<int> pDelay, pRev, pPhaser, pFilter, pBrush;
};
