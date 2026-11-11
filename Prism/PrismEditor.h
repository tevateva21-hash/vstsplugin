#pragma once
#include "PrismProcessor.h"
#include "PsyUI.h"

class WaveDisplay : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit WaveDisplay (PrismProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { drag (ePos (getMouseXYRelative().x), true); }
    void mouseDrag (const juce::MouseEvent&) override { drag (ePos (getMouseXYRelative().x), false); }
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;
private:
    float ePos (int x) const { return juce::jlimit (0.f, 1.f, (float) x / (float) std::max (1, getWidth())); }
    void drag (float u, bool first);
    PrismProcessor& proc; bool moveEnd = false;
};

class FilterDisplay : public juce::Component
{
public:
    explicit FilterDisplay (PrismProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { setFromMouse (e); }
    void mouseDrag (const juce::MouseEvent& e) override { setFromMouse (e); }
private:
    void setFromMouse (const juce::MouseEvent& e);
    float magDb (int type, float f, float fc, float res) const;
    PrismProcessor& proc;
};

class SeqGrid : public juce::Component
{
public:
    explicit SeqGrid (PrismProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { setAt (e); }
    void mouseDrag (const juce::MouseEvent& e) override { setAt (e); }
private:
    void setAt (const juce::MouseEvent&);
    PrismProcessor& proc;
};

class PrismEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PrismEditor (PrismProcessor&);
    ~PrismEditor() override { setLookAndFeel (nullptr); }
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { wave.repaint(); filterDisp.repaint(); seqGrid.repaint(); }
    void setTab (int idx);
    Panel* addPanel (const juce::String& title, const juce::String& power = {});

    PrismProcessor& proc;
    PsyLookAndFeel lnf;
    juce::TooltipWindow tooltip { this, 400 };
    PresetBar presetBar;
    Knob master;
    TabBar tabs;
    juce::TextButton loadBtn { "LOAD SAMPLE" }, shapeBtn { "FORMES SEQ..." };
    std::unique_ptr<juce::FileChooser> chooser;
    WaveDisplay wave; FilterDisplay filterDisp; SeqGrid seqGrid;
    std::vector<std::unique_ptr<Panel>> panels;
    juce::MidiKeyboardComponent keyboard;
};

