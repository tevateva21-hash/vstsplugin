#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace KinoColors
{
const juce::Colour bg       (0xfff5f5f5); // Gris perle clair
const juce::Colour card     (0xffffffff); // Blanc pur (Clean UI)
const juce::Colour border   (0xffe0dfe4); // Bordure fine clinique
const juce::Colour red      (0xffff1b24); // Rouge vibrant "Cinema"
const juce::Colour redSoft  (0xffffe5e7); // Teinte rouge legere pour survol
const juce::Colour text     (0xff302e2e); // Gris anthracite profond
const juce::Colour textDim  (0xff7d7979); // Gris secondaire lisible
const juce::Colour ink      (0xff000000); // Noir d'encre BD (contours epais)
const juce::Colour cyan3D   (0xff00e5ff); // Bleu Cyan Pop 3D
}

class KinoLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KinoLookAndFeel()
    {
        setColour (juce::ComboBox::backgroundColourId, KinoColors::card);
        setColour (juce::ComboBox::textColourId, KinoColors::text);
        setColour (juce::ComboBox::outlineColourId, KinoColors::border);
        setColour (juce::ComboBox::arrowColourId, KinoColors::red);
        setColour (juce::PopupMenu::backgroundColourId, KinoColors::card);
        setColour (juce::PopupMenu::textColourId, KinoColors::text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, KinoColors::red);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::TextButton::buttonColourId, KinoColors::card);
        setColour (juce::TextButton::buttonOnColourId, KinoColors::red);
        setColour (juce::TextButton::textColourOffId, KinoColors::text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::Label::textColourId, KinoColors::text);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float s, float e, juce::Slider&) override
    {
        auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float d = std::min (r.getWidth(), r.getHeight());
        auto c = r.getCentre();
        const float rad = d * 0.5f;
        const float ang = s + pos * (e - s);

        // Piste gris perle
        juce::Path track;
        track.addCentredArc (c.x, c.y, rad - 2.5f, rad - 2.5f, 0, s, e, true);
        g.setColour (KinoColors::bg.darker (0.06f));
        g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Arc actif rouge cinema
        juce::Path val;
        val.addCentredArc (c.x, c.y, rad - 2.5f, rad - 2.5f, 0, s, ang, true);
        g.setColour (KinoColors::red);
        g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Corps blanc clinique avec ombre douce
        const float bodyRad = rad * 0.66f;
        g.setColour (juce::Colours::black.withAlpha (0.08f));
        g.fillEllipse (c.x - bodyRad, c.y - bodyRad + 2.0f, bodyRad * 2.0f, bodyRad * 2.0f);
        g.setColour (KinoColors::card);
        g.fillEllipse (c.x - bodyRad, c.y - bodyRad, bodyRad * 2.0f, bodyRad * 2.0f);
        g.setColour (KinoColors::border.darker (0.1f));
        g.drawEllipse (c.x - bodyRad, c.y - bodyRad, bodyRad * 2.0f, bodyRad * 2.0f, 1.2f);

        // Indicateur rouge
        g.setColour (KinoColors::red);
        g.drawLine (c.x + std::sin (ang) * bodyRad * 0.25f, c.y - std::cos (ang) * bodyRad * 0.25f,
                    c.x + std::sin (ang) * (bodyRad - 2.0f), c.y - std::cos (ang) * (bodyRad - 2.0f), 2.4f);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        auto r = juce::Rectangle<float> (0.5f, 0.5f, (float) w - 1.0f, (float) h - 1.0f);
        g.setColour (KinoColors::card);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (KinoColors::border);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        juce::Path a;
        const float cx = (float) w - 11.0f, cy = h * 0.5f;
        a.addTriangle (cx - 3.5f, cy - 2.0f, cx + 3.5f, cy - 2.0f, cx, cy + 3.0f);
        g.setColour (KinoColors::red);
        g.fillPath (a);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.75f);
        const bool on = b.getToggleState();
        juce::Colour fill = on ? KinoColors::red : (down ? KinoColors::bg.darker (0.05f) : (over ? KinoColors::redSoft : KinoColors::card));
        g.setColour (fill);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (on ? KinoColors::red : (over ? KinoColors::red.withAlpha (0.5f) : KinoColors::border));
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
    }
};

class KinoKnob : public juce::Component
{
public:
    KinoKnob (juce::AudioProcessorValueTreeState& s, const juce::String& id, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setPopupDisplayEnabled (true, true, nullptr);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
        label.setColour (juce::Label::textColourId, KinoColors::text);
        addAndMakeVisible (label);
        att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (s, id, slider);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromBottom (14));
        slider.setBounds (r.reduced (1));
    }

    juce::Slider slider;
private:
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// Spectateur 3D BD dessine au trait noir epais (#000000) + lunettes rouge/cyan (#FF1B24 / #00E5FF)
class SpectatorAvatar : public juce::Component
{
public:
    SpectatorAvatar (int styleIdx, const juce::String& labelText, std::function<void()> onClickCb)
        : style (styleIdx), caption (labelText), onClick (std::move (onClickCb))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void setPulse (float p)
    {
        if (std::abs (pulse - p) > 0.02f) { pulse = p; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool hover = isMouseOver();

        // Carte blanche clinique avec ombre douce
        g.setColour (juce::Colours::black.withAlpha (0.05f));
        g.fillRoundedRectangle (r.reduced (2.0f).translated (0.0f, 2.0f), 8.0f);
        g.setColour (hover ? KinoColors::redSoft.withAlpha (0.5f) : KinoColors::card);
        g.fillRoundedRectangle (r.reduced (2.0f), 8.0f);
        g.setColour (hover ? KinoColors::red : KinoColors::border);
        g.drawRoundedRectangle (r.reduced (2.0f), 8.0f, hover ? 1.5f : 1.0f);

        // Dessin Cartoon BD du spectateur qui rebondit au rythme MIDI
        const float bounce = -pulse * 3.5f;
        const float cx = r.getCentreX();
        const float cy = r.getY() + 25.0f + bounce;

        // Tete cartoon avec trait noir epais 2.5px
        const float hr = 14.5f;
        g.setColour (juce::Colour (0xfffaf6f0));
        g.fillEllipse (cx - hr, cy - hr, hr * 2.0f, hr * 2.0f);
        g.setColour (KinoColors::ink);
        g.drawEllipse (cx - hr, cy - hr, hr * 2.0f, hr * 2.0f, 2.4f);

        // Coiffure / chapeau varie selon le style (0..3)
        juce::Path hair;
        if (style == 0) // Mec coupe punk/pic
        {
            hair.startNewSubPath (cx - 12.0f, cy - 8.0f);
            hair.lineTo (cx - 8.0f, cy - 20.0f);
            hair.lineTo (cx, cy - 15.0f);
            hair.lineTo (cx + 7.0f, cy - 21.0f);
            hair.lineTo (cx + 12.0f, cy - 8.0f);
        }
        else if (style == 1) // Bob / coupe cinema retro
        {
            hair.addCentredArc (cx, cy - 2.0f, hr + 2.0f, hr + 1.0f, 0.0f, -2.5f, 2.5f, true);
        }
        else if (style == 2) // Casquette / visiere
        {
            hair.addRoundedRectangle (cx - 14.0f, cy - 17.0f, 28.0f, 8.0f, 3.0f);
        }
        else // Cheveux boucles / goofy
        {
            hair.addEllipse (cx - 16.0f, cy - 19.0f, 12.0f, 10.0f);
            hair.addEllipse (cx - 5.0f, cy - 21.0f, 12.0f, 10.0f);
            hair.addEllipse (cx + 5.0f, cy - 19.0f, 12.0f, 10.0f);
        }
        g.setColour (KinoColors::text);
        g.fillPath (hair);
        g.setColour (KinoColors::ink);
        g.strokePath (hair, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Lunettes 3D anaglyphes (verre gauche Rouge #FF1B24, verre droit Cyan #00E5FF)
        const float gy = cy - 3.0f;
        juce::Rectangle<float> leftLens (cx - 12.5f, gy - 4.0f, 11.0f, 8.0f);
        juce::Rectangle<float> rightLens (cx + 1.5f, gy - 4.0f, 11.0f, 8.0f);

        // Monture blanche
        g.setColour (KinoColors::card);
        g.fillRoundedRectangle (cx - 14.5f, gy - 5.5f, 29.0f, 11.0f, 2.0f);
        g.setColour (KinoColors::ink);
        g.drawRoundedRectangle (cx - 14.5f, gy - 5.5f, 29.0f, 11.0f, 2.0f, 2.2f);

        // Verres Rouge & Cyan
        g.setColour (KinoColors::red);
        g.fillRect (leftLens);
        g.setColour (KinoColors::cyan3D);
        g.fillRect (rightLens);
        g.setColour (KinoColors::ink);
        g.drawRect (leftLens, 1.5f);
        g.drawRect (rightLens, 1.5f);

        // Sourire expressif BD
        juce::Path smile;
        if (pulse > 0.3f)
            smile.addEllipse (cx - 4.0f, cy + 5.0f, 8.0f, 5.5f); // bouche ouverte "ooh!"
        else
            smile.addCentredArc (cx, cy + 5.0f, 5.5f, 3.5f, 0.0f, 0.3f, 2.84f, true);
        g.setColour (KinoColors::ink);
        g.strokePath (smile, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Label preset en bas
        g.setColour (hover ? KinoColors::red : KinoColors::text);
        g.setFont (juce::Font (juce::FontOptions (9.5f).withStyle ("Bold")));
        g.drawText (caption, r.removeFromBottom (16.0f).toNearestInt(), juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }

private:
    int style;
    juce::String caption;
    std::function<void()> onClick;
    float pulse = 0.0f;
};
