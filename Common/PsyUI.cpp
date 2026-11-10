#include "PsyUI.h"
using namespace PsyUI;

PsyLookAndFeel::PsyLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panelHi);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::arrowColourId, red);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, redDark);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, panelHi);
    setColour (juce::TextButton::buttonOnColourId, redDark);
    setColour (juce::TextButton::textColourOffId, textDim);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Label::textColourId, textDim);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, panelHi);
    setColour (juce::Slider::textBoxOutlineColourId, line);
    setColour (juce::Slider::trackColourId, red);
    setColour (juce::TooltipWindow::backgroundColourId, panel);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::TextEditor::backgroundColourId, panelHi);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, line);
}

void PsyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float s, float e, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = std::min (r.getWidth(), r.getHeight());
    auto c = r.getCentre(); const float rad = d * 0.5f;
    const float ang = s + pos * (e - s);
    g.setColour (panelHi); g.fillEllipse (c.x - rad * 0.72f, c.y - rad * 0.72f, rad * 1.44f, rad * 1.44f);
    juce::Path track; track.addCentredArc (c.x, c.y, rad - 1.5f, rad - 1.5f, 0, s, e, true);
    g.setColour (line); g.strokePath (track, juce::PathStrokeType (2.5f));
    juce::Path val; val.addCentredArc (c.x, c.y, rad - 1.5f, rad - 1.5f, 0, s, ang, true);
    g.setColour (red); g.strokePath (val, juce::PathStrokeType (2.5f));
    g.setColour (redHot);
    g.drawLine (c.x + std::sin (ang) * rad * 0.25f, c.y - std::cos (ang) * rad * 0.25f,
                c.x + std::sin (ang) * rad * 0.68f, c.y - std::cos (ang) * rad * 0.68f, 2.0f);
}

void PsyLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    g.setColour (panelHi); g.fillRoundedRectangle (0, 0, (float) w, (float) h, 2.0f);
    g.setColour (line); g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1, (float) h - 1, 2.0f, 1.0f);
    juce::Path a; const float cx = (float) w - 10, cy = h * 0.5f;
    a.addTriangle (cx - 3.5f, cy - 2, cx + 3.5f, cy - 2, cx, cy + 3);
    g.setColour (red); g.fillPath (a);
}

void PsyLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (b.getToggleState() ? red.darker (0.2f) : (over ? panelHi.brighter (0.1f) : panelHi));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (b.getToggleState() ? red : line); g.drawRoundedRectangle (r, 2.0f, 1.0f);
}

void PsyLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearHorizontal) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s); return; }
    const float cy = y + h * 0.5f;
    g.setColour (line); g.fillRect ((float) x, cy - 1.5f, (float) w, 3.0f);
    g.setColour (red); g.fillRect ((float) x, cy - 1.5f, pos - x, 3.0f);
    g.setColour (redHot); g.fillRect (pos - 3.0f, cy - 7.0f, 6.0f, 14.0f);
}

Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name, bool withLabel) : hasLabel (withLabel)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification); label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (10.5f))); label.setInterceptsMouseClicks (false, false);
    if (hasLabel) addAndMakeVisible (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}
void Knob::resized()
{
    auto r = getLocalBounds();
    if (hasLabel) label.setBounds (r.removeFromBottom (13));
    slider.setBounds (r);
}

// ---------------------------------------------------------------- Panel
Panel::Panel (juce::AudioProcessorValueTreeState& a, const juce::String& t, const juce::String& powerId) : apvts (a), title (t)
{
    if (powerId.isNotEmpty())
    {
        hasPower = true; power.setClickingTogglesState (true); addAndMakeVisible (power);
        att.push_back (std::make_shared<juce::AudioProcessorValueTreeState::ButtonAttachment> (a, powerId, power));
    }
}
void Panel::addKnob (const juce::String& id, const juce::String& label)
{
    Cell c; c.w = 58; c.comp = std::make_unique<Knob> (apvts, id, label);
    addAndMakeVisible (*c.comp); cells.push_back (std::move (c));
}
void Panel::addCombo (const juce::String& id, const juce::String& label)
{
    Cell c; c.w = 100; auto cb = std::make_unique<juce::ComboBox>();
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id))) cb->addItemList (p->choices, 1);
    att.push_back (std::make_shared<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, *cb));
    c.label = std::make_unique<juce::Label>(); c.label->setText (label, juce::dontSendNotification);
    c.label->setJustificationType (juce::Justification::centred); c.label->setFont (juce::Font (juce::FontOptions (10.5f)));
    addAndMakeVisible (*cb); addAndMakeVisible (*c.label); c.comp = std::move (cb); cells.push_back (std::move (c));
}
void Panel::addToggle (const juce::String& id, const juce::String& label)
{
    Cell c; c.w = 58; auto b = std::make_unique<juce::TextButton> (label); b->setClickingTogglesState (true);
    att.push_back (std::make_shared<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, *b));
    addAndMakeVisible (*b); c.comp = std::move (b); cells.push_back (std::move (c));
}
int Panel::preferredWidth() const
{
    int w = 16; for (auto& c : cells) w += c.w;
    return std::max (w, hasPower ? 130 : 90);
}
void Panel::paint (juce::Graphics& g)
{
    g.setColour (PsyUI::panel); g.fillRoundedRectangle (getLocalBounds().toFloat(), 3.0f);
    g.setColour (line); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 3.0f, 1.0f);
    g.setColour (red); g.setFont (juce::Font (juce::FontOptions (11.5f).withStyle ("Bold")));
    g.drawText (title, 10, 4, getWidth() - 60, 16, juce::Justification::left);
}
void Panel::resized()
{
    if (hasPower) power.setBounds (getWidth() - 40, 4, 32, 16);
    int x = 8;
    for (auto& c : cells)
    {
        if (dynamic_cast<Knob*> (c.comp.get())) c.comp->setBounds (x, 22, c.w, 82);
        else if (dynamic_cast<juce::ComboBox*> (c.comp.get()))
        {
            c.comp->setBounds (x + 2, 22 + 22, c.w - 4, 24);
            c.label->setBounds (x, 22 + 66, c.w, 14);
        }
        else c.comp->setBounds (x + 2, 22 + 22, c.w - 4, 24);
        x += c.w;
    }
}

int layoutRow (int x, int y, const std::vector<Panel*>& row)
{
    for (auto* p : row) { p->setBounds (x, y, p->preferredWidth(), Panel::height); x += p->preferredWidth() + 8; }
    return x;
}
