// =============================================================================
//  PLEXIS Labs — Charlie
//  Controls.h : contrôles de l'interface reliés aux paramètres (APVTS)
//    Knob        : potentiomètre + étiquette
//    EnvFader    : fader vertical + étiquette
//    LedToggle   : interrupteur à DEL (paramètre booléen)
//    RadioLeds   : groupe de DEL exclusives (paramètre à choix)  ex. MAN/ENV/LFO
//    IconSelector: bouton à icône qui fait défiler les choix (formes d'onde, filtres)
//    LcdChoice   : afficheur LCD déroulant (paramètre à choix)
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PlexisLookAndFeel.h"

using APVTS = juce::AudioProcessorValueTreeState;

// ---------------------------------------------------------------------------
class Knob : public juce::Component
{
public:
    Knob (APVTS& s, const char* paramId, const juce::String& labelText) : label (labelText)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        slider.setPopupDisplayEnabled (true, false, nullptr);
        slider.setVelocityBasedMode (false);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);
        attachment = std::make_unique<APVTS::SliderAttachment> (s, paramId, slider);
        if (auto* p = s.getParameter (paramId)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        slider.setTooltip (s.getParameter (paramId)->getName (64));
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (PlexisTheme::text);
        g.setFont (PlexisTheme::font (11.0f));
        g.drawText (label, getLocalBounds().removeFromBottom (14), juce::Justification::centred);
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (14)); }

    juce::Slider slider;
private:
    juce::String label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// ---------------------------------------------------------------------------
class EnvFader : public juce::Component
{
public:
    EnvFader (APVTS& s, const char* paramId, const juce::String& labelText) : label (labelText)
    {
        slider.setSliderStyle (juce::Slider::LinearVertical);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setPopupDisplayEnabled (true, false, nullptr);
        addAndMakeVisible (slider);
        attachment = std::make_unique<APVTS::SliderAttachment> (s, paramId, slider);
        if (auto* p = s.getParameter (paramId)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (PlexisTheme::text);
        g.setFont (PlexisTheme::font (11.0f));
        g.drawText (label, getLocalBounds().removeFromBottom (14), juce::Justification::centred);
    }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (16)); }
private:
    juce::Slider slider;
    juce::String label;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

// ---------------------------------------------------------------------------
class LedToggle : public juce::Component
{
public:
    LedToggle (APVTS& s, const char* paramId, const juce::String& text)
    {
        button.setButtonText (text);
        addAndMakeVisible (button);
        attachment = std::make_unique<APVTS::ButtonAttachment> (s, paramId, button);
        button.setTooltip (s.getParameter (paramId)->getName (64));
    }
    void resized() override { button.setBounds (getLocalBounds()); }
private:
    juce::ToggleButton button;
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

// ---------------------------------------------------------------------------
class RadioLeds : public juce::Component, public juce::SettableTooltipClient
{
public:
    RadioLeds (APVTS& s, const char* paramId, const juce::StringArray& labels) : names (labels)
    {
        auto* p = s.getParameter (paramId);
        attachment = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v) { selected = juce::roundToInt (v); repaint(); }, nullptr);
        attachment->sendInitialUpdate();
        setTooltip (p->getName (64));
    }
    void paint (juce::Graphics& g) override
    {
        const float rowH = (float) getHeight() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            auto row = juce::Rectangle<float> (0.0f, rowH * (float) i, (float) getWidth(), rowH);
            PlexisLookAndFeel::drawLed (g, { 2.0f, row.getCentreY() - 6.0f, 22.0f, 12.0f }, i == selected, i == hoverRow);
            g.setColour (i == selected ? PlexisTheme::text : PlexisTheme::textDim);
            g.setFont (PlexisTheme::font (11.5f));
            g.drawText (names[i], row.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
        }
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int row = juce::jlimit (0, names.size() - 1, (int) (e.position.y / ((float) getHeight() / (float) names.size())));
        attachment->setValueAsCompleteGesture ((float) row);
    }
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int row = (int) (e.position.y / ((float) getHeight() / (float) names.size()));
        if (row != hoverRow) { hoverRow = row; repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override { hoverRow = -1; repaint(); }
private:
    juce::StringArray names;
    int selected = 0, hoverRow = -1;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

// ---------------------------------------------------------------------------
class IconSelector : public juce::Component, public juce::SettableTooltipClient
{
public:
    enum class Kind { OscWave, FilterType, LfoWave };

    IconSelector (APVTS& s, const char* paramId, Kind k) : kind (k)
    {
        param = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (paramId));
        jassert (param != nullptr);
        attachment = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v) { index = juce::roundToInt (v); repaint(); }, nullptr);
        attachment->sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace PlexisTheme;
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (panelShadow.withAlpha (0.6f)); g.fillRoundedRectangle (r.translated (0, 2), 4.0f);
        juce::ColourGradient bg (Colour (0xff3a3e44), r.getX(), r.getY(), Colour (0xff1d1f22), r.getX(), r.getBottom(), false);
        g.setGradientFill (bg); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (hover ? accent : panelEdge); g.drawRoundedRectangle (r, 4.0f, 1.0f);

        auto icon = r.reduced (9.0f, 8.0f);
        juce::Path p = makeIcon (icon);
        g.setColour (accent.withAlpha (0.25f));
        g.strokePath (p, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (kind == Kind::FilterType)
        {
            g.setFont (font (10.0f));
            g.setColour (text);
            const char* tag[] = { "24", "12", "BP" };
            g.drawText (tag[juce::jlimit (0, 2, index)], r.removeFromTop (14).removeFromRight (20), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int n = param->choices.size();
        if (e.mods.isPopupMenu())
        {
            juce::PopupMenu m;
            for (int i = 0; i < n; ++i) m.addItem (i + 1, param->choices[i], true, i == index);
            m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [this] (int r) { if (r > 0) attachment->setValueAsCompleteGesture ((float) (r - 1)); });
            return;
        }
        attachment->setValueAsCompleteGesture ((float) ((index + 1) % n));
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override  { hover = false; repaint(); }

private:
    juce::Path makeIcon (juce::Rectangle<float> r) const
    {
        juce::Path p;
        const float x0 = r.getX(), x1 = r.getRight(), y0 = r.getY(), y1 = r.getBottom(), ym = r.getCentreY(), w = r.getWidth();
        auto sine = [&] (float phaseOff)
        {
            for (int i = 0; i <= 40; ++i)
            {
                const float t = (float) i / 40.0f;
                const float y = ym - (y1 - y0) * 0.5f * std::sin (juce::MathConstants<float>::twoPi * t + phaseOff);
                if (i == 0) p.startNewSubPath (x0 + t * w, y); else p.lineTo (x0 + t * w, y);
            }
        };

        if (kind == Kind::OscWave)
        {
            switch (index)
            {
                case 0: sine (0.0f); break;
                case 1: p.startNewSubPath (x0, y1); p.lineTo (x0 + w * 0.5f, y0); p.lineTo (x1, y1); break;
                case 2: p.startNewSubPath (x0, y1); p.lineTo (x0 + w * 0.5f, y0); p.lineTo (x0 + w * 0.5f, y1); p.lineTo (x1, y0); break;
                default: p.startNewSubPath (x0, y1); p.lineTo (x0, y0); p.lineTo (x0 + w * 0.5f, y0); p.lineTo (x0 + w * 0.5f, y1); p.lineTo (x1, y1); p.lineTo (x1, y0); break;
            }
        }
        else if (kind == Kind::FilterType)
        {
            if (index == 2) // passe-bande
            {
                p.startNewSubPath (x0, y1); p.cubicTo (x0 + w * 0.3f, y1, x0 + w * 0.35f, y0, x0 + w * 0.5f, y0);
                p.cubicTo (x0 + w * 0.65f, y0, x0 + w * 0.7f, y1, x1, y1);
            }
            else // passe-bas (pente plus raide en 24 dB)
            {
                const float knee = index == 0 ? 0.55f : 0.45f;
                p.startNewSubPath (x0, ym - (y1 - y0) * 0.2f);
                p.lineTo (x0 + w * knee, ym - (y1 - y0) * 0.2f);
                p.quadraticTo (x0 + w * (knee + 0.12f), y0 - 2.0f, x0 + w * (knee + 0.18f), ym);
                p.lineTo (index == 0 ? x0 + w * 0.85f : x1, y1);
            }
        }
        else // LFO
        {
            switch (index)
            {
                case 0: sine (0.0f); break;
                case 1: p.startNewSubPath (x0, ym); p.lineTo (x0 + w * 0.25f, y0); p.lineTo (x0 + w * 0.75f, y1); p.lineTo (x1, ym); break;
                case 2: p.startNewSubPath (x0, y1); p.lineTo (x0 + w * 0.5f, y0); p.lineTo (x0 + w * 0.5f, y1); p.lineTo (x1, y0); break;
                case 3: p.startNewSubPath (x0, y0); p.lineTo (x0 + w * 0.5f, y1); p.lineTo (x0 + w * 0.5f, y0); p.lineTo (x1, y1); break;
                case 4: p.startNewSubPath (x0, y1); p.lineTo (x0, y0); p.lineTo (x0 + w * 0.5f, y0); p.lineTo (x0 + w * 0.5f, y1); p.lineTo (x1, y1); p.lineTo (x1, y0); break;
                case 5:
                {
                    const float lv[] = { 0.3f, 0.9f, 0.1f, 0.6f, 0.75f };
                    p.startNewSubPath (x0, y0 + (y1 - y0) * lv[0]);
                    for (int i = 0; i < 5; ++i)
                    {
                        const float xa = x0 + w * (float) i / 5.0f, xb = x0 + w * (float) (i + 1) / 5.0f, yy = y0 + (y1 - y0) * lv[i];
                        p.lineTo (xa, yy); p.lineTo (xb, yy);
                    }
                    break;
                }
                default:
                {
                    p.startNewSubPath (x0, ym);
                    p.cubicTo (x0 + w * 0.2f, y0, x0 + w * 0.35f, y0 + 4.0f, x0 + w * 0.5f, ym + 3.0f);
                    p.cubicTo (x0 + w * 0.65f, y1, x0 + w * 0.8f, y1 - 2.0f, x1, y0 + 6.0f);
                    break;
                }
            }
        }
        return p;
    }

    Kind kind;
    juce::AudioParameterChoice* param = nullptr;
    std::unique_ptr<juce::ParameterAttachment> attachment;
    int index = 0;
    bool hover = false;
};

// ---------------------------------------------------------------------------
class LcdChoice : public juce::Component
{
public:
    LcdChoice (APVTS& s, const char* paramId)
    {
        if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (paramId)))
            box.addItemList (p->choices, 1);
        addAndMakeVisible (box);
        attachment = std::make_unique<APVTS::ComboBoxAttachment> (s, paramId, box);
        box.setTooltip (s.getParameter (paramId)->getName (64));
    }
    void resized() override { box.setBounds (getLocalBounds()); }
private:
    juce::ComboBox box;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};
