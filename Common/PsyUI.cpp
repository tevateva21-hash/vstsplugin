#include "PsyUI.h"
using namespace PsyUI;

PsyLookAndFeel::PsyLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, text);
    setColour (juce::ComboBox::outlineColourId, line);
    setColour (juce::ComboBox::arrowColourId, redHot);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, redDark);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::buttonColourId, panel);
    setColour (juce::TextButton::buttonOnColourId, redDark);
    setColour (juce::TextButton::textColourOffId, text);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::Label::textColourId, textDim);
    setColour (juce::Slider::textBoxTextColourId, text);
    setColour (juce::Slider::textBoxBackgroundColourId, panel);
    setColour (juce::Slider::textBoxOutlineColourId, line);
    setColour (juce::Slider::trackColourId, red);
    setColour (juce::TooltipWindow::backgroundColourId, panel);
    setColour (juce::TooltipWindow::textColourId, text);
    setColour (juce::AlertWindow::backgroundColourId, panel);
    setColour (juce::AlertWindow::textColourId, text);
    setColour (juce::TextEditor::backgroundColourId, panel);
    setColour (juce::TextEditor::textColourId, text);
    setColour (juce::TextEditor::outlineColourId, line);
}

void PsyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float s, float e, juce::Slider&)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float d = std::min (r.getWidth(), r.getHeight());
    auto c = r.getCentre();
    const float rad = d * 0.5f;
    const float ang = s + pos * (e - s);

    // Ticks sérigraphiés blanc cassé usé autour de la couronne
    g.setColour (textDim.withAlpha (0.45f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = s + (float) i / 10.0f * (e - s);
        const float r0 = rad - 1.0f, r1 = rad + 2.0f;
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0,
                    c.x + std::sin (a) * r1, c.y - std::cos (a) * r1, 1.2f);
    }

    // Piste de fond noir d'encre net
    juce::Path track;
    track.addCentredArc (c.x, c.y, rad - 3.5f, rad - 3.5f, 0, s, e, true);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Halo et arc de valeur rouge cinéma
    juce::Path val;
    val.addCentredArc (c.x, c.y, rad - 3.5f, rad - 3.5f, 0, s, ang, true);
    g.setColour (red.withAlpha (0.35f));
    g.strokePath (val, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (redHot);
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Corps du potard : Noir d'encre mat avec ombre portée franche style comic et liseré gris terreux
    const float bodyRad = rad * 0.68f;
    g.setColour (juce::Colours::black);
    g.fillEllipse (c.x - bodyRad + 1.5f, c.y - bodyRad + 2.5f, bodyRad * 2.0f, bodyRad * 2.0f); // ombre dure
    g.setColour (panel);
    g.fillEllipse (c.x - bodyRad, c.y - bodyRad, bodyRad * 2.0f, bodyRad * 2.0f);
    g.setColour (grayEarth);
    g.drawEllipse (c.x - bodyRad, c.y - bodyRad, bodyRad * 2.0f, bodyRad * 2.0f, 1.2f);

    // Curseur lumineux rouge cinéma
    g.setColour (redHot);
    g.drawLine (c.x + std::sin (ang) * bodyRad * 0.20f, c.y - std::cos (ang) * bodyRad * 0.20f,
                c.x + std::sin (ang) * (bodyRad - 2.0f), c.y - std::cos (ang) * (bodyRad - 2.0f), 2.5f);
}

void PsyLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f);
    g.setColour (panel); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (line); g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path a; const float cx = (float) w - 11.0f, cy = h * 0.5f;
    a.addTriangle (cx - 4.0f, cy - 2.0f, cx + 4.0f, cy - 2.0f, cx, cy + 3.0f);
    g.setColour (redHot); g.fillPath (a);
}

void PsyLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.75f);
    const bool on = b.getToggleState();
    juce::Colour base = on ? redDark : (down ? juce::Colours::black : (over ? panelHi : panel));
    if (on)
    {
        g.setColour (red.withAlpha (0.3f));
        g.fillRoundedRectangle (r.expanded (1.0f), 4.5f);
    }
    g.setColour (base);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? redHot : (over ? textDim : line));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

void PsyLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearHorizontal) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s); return; }
    const float cy = y + h * 0.5f;
    g.setColour (juce::Colours::black); g.fillRoundedRectangle ((float) x, cy - 2.5f, (float) w, 5.0f, 2.5f);
    g.setColour (line); g.drawRoundedRectangle ((float) x, cy - 2.5f, (float) w, 5.0f, 2.5f, 1.0f);
    g.setColour (red); g.fillRoundedRectangle ((float) x, cy - 2.0f, std::max (0.0f, pos - x), 4.0f, 2.0f);
    g.setColour (redHot); g.fillRoundedRectangle (pos - 4.0f, cy - 8.0f, 8.0f, 16.0f, 2.5f);
    g.setColour (text); g.drawRoundedRectangle (pos - 4.0f, cy - 8.0f, 8.0f, 16.0f, 2.5f, 1.0f);
}

Knob::Knob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name, bool withLabel) : hasLabel (withLabel)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled (true, true, nullptr);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (slider);
    label.setText (name, juce::dontSendNotification); label.setJustificationType (juce::Justification::centred);
    label.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, text);
    label.setInterceptsMouseClicks (false, false);
    if (hasLabel) addAndMakeVisible (label);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
}
void Knob::resized()
{
    auto r = getLocalBounds();
    if (hasLabel) label.setBounds (r.removeFromBottom (16));
    slider.setBounds (r.reduced (1));
}

// ---------------------------------------------------------------- TabBar
TabBar::TabBar (std::initializer_list<const char*> names, std::function<void (int)> onSelect) : cb (std::move (onSelect))
{
    int idx = 0;
    for (auto* n : names)
    {
        auto b = std::make_unique<juce::TextButton> (n);
        const int i = idx++;
        b->onClick = [this, i] { select (i); };
        addAndMakeVisible (*b);
        btns.push_back (std::move (b));
    }
    select (0);
}

void TabBar::select (int idx)
{
    active = idx;
    for (int i = 0; i < (int) btns.size(); ++i)
        btns[(size_t) i]->setToggleState (i == active, juce::dontSendNotification);
    if (cb) cb (active);
}

void TabBar::resized()
{
    if (btns.empty()) return;
    auto r = getLocalBounds();
    const int gap = 6;
    const int w = (r.getWidth() - gap * ((int) btns.size() - 1)) / (int) btns.size();
    for (size_t i = 0; i < btns.size(); ++i)
    {
        btns[i]->setBounds (r.removeFromLeft (w));
        r.removeFromLeft (gap);
    }
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
void Panel::addKnob (const juce::String& id, const juce::String& label, int width)
{
    Cell c; c.w = width; c.comp = std::make_unique<Knob> (apvts, id, label);
    addAndMakeVisible (*c.comp); cells.push_back (std::move (c));
}
void Panel::addCombo (const juce::String& id, const juce::String& label, int width)
{
    Cell c; c.w = width; auto cb = std::make_unique<juce::ComboBox>();
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id))) cb->addItemList (p->choices, 1);
    att.push_back (std::make_shared<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (apvts, id, *cb));
    c.label = std::make_unique<juce::Label>(); c.label->setText (label, juce::dontSendNotification);
    c.label->setJustificationType (juce::Justification::centred);
    c.label->setFont (juce::Font (juce::FontOptions (10.5f).withStyle ("Bold")));
    c.label->setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (*cb); addAndMakeVisible (*c.label); c.comp = std::move (cb); cells.push_back (std::move (c));
}
void Panel::addToggle (const juce::String& id, const juce::String& label, int width)
{
    Cell c; c.w = width; auto b = std::make_unique<juce::TextButton> (label); b->setClickingTogglesState (true);
    att.push_back (std::make_shared<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, id, *b));
    addAndMakeVisible (*b); c.comp = std::move (b); cells.push_back (std::move (c));
}
int Panel::preferredWidth() const
{
    int w = 24; for (auto& c : cells) w += c.w;
    return std::max (w, hasPower ? 150 : 110);
}
void Panel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    // Ombre décalée franche style comic book (sans flou, pas d'anthracite lourd)
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r.translated (1.5f, 2.5f), 5.0f);

    // Façade de module noir d'encre épurée
    g.setColour (PsyUI::panel);
    g.fillRoundedRectangle (r, 5.0f);

    // Contour d'encrage franc
    g.setColour (line);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);

    // Bandeau de titre en haut du panneau
    auto head = r.removeFromTop (24.0f);
    g.setColour (panelHi.withAlpha (0.6f));
    g.fillRoundedRectangle (head.getX() + 1.0f, head.getY() + 1.0f, head.getWidth() - 2.0f, head.getHeight(), 4.0f);
    g.setColour (red);
    g.fillRoundedRectangle (head.getX() + 8.0f, head.getY() + 5.0f, 3.5f, 14.0f, 1.5f);

    g.setColour (line);
    g.drawHorizontalLine (24, 1.0f, (float) getWidth() - 1.0f);

    g.setColour (text);
    g.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    g.drawText (title, 18, 4, getWidth() - (hasPower ? 64 : 24), 16, juce::Justification::centredLeft);
}
void Panel::resized()
{
    if (hasPower) power.setBounds (getWidth() - 46, 3, 38, 18);
    if (cells.empty()) return;

    int sumW = 0;
    for (auto& c : cells) sumW += c.w;
    const int avail = std::max (sumW, getWidth() - 20);
    const float scale = (float) avail / (float) std::max (1, sumW);
    const int bodyY = 28;
    const int bodyH = getHeight() - bodyY - 6;

    float fx = 10.0f;
    for (auto& c : cells)
    {
        const int cw = (int) std::round (c.w * scale);
        const int ix = (int) std::round (fx);
        if (dynamic_cast<Knob*> (c.comp.get()))
        {
            c.comp->setBounds (ix, bodyY, cw, bodyH);
        }
        else if (dynamic_cast<juce::ComboBox*> (c.comp.get()))
        {
            c.comp->setBounds (ix + 4, bodyY + (bodyH - 42) / 2, cw - 8, 26);
            c.label->setBounds (ix, bodyY + bodyH - 16, cw, 16);
        }
        else
        {
            c.comp->setBounds (ix + 4, bodyY + (bodyH - 28) / 2, cw - 8, 28);
        }
        fx += c.w * scale;
    }
}

int layoutRow (int x, int y, const std::vector<Panel*>& row, int totalWidth, int rowHeight)
{
    if (row.empty()) return x;
    const int gap = 10;
    int sumPref = 0;
    for (auto* p : row) sumPref += p->preferredWidth();
    const int avail = totalWidth > 0 ? std::max (sumPref, totalWidth - gap * ((int) row.size() - 1)) : sumPref;

    int curX = x;
    for (size_t i = 0; i < row.size(); ++i)
    {
        int w = (i + 1 == row.size() && totalWidth > 0)
                    ? (x + totalWidth - curX)
                    : (int) std::round ((double) row[i]->preferredWidth() * avail / std::max (1, sumPref));
        row[i]->setBounds (curX, y, w, rowHeight);
        curX += w + gap;
    }
    return curX;
}

