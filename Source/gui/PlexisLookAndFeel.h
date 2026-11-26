// =============================================================================
//  PLEXIS Labs — LookAndFeel commun
//  Toutes les couleurs de la marque sont regroupées dans PlexisTheme : pour aligner
//  Charlie pixel pour pixel sur FlexStrip, il suffit de reporter ici les valeurs
//  du LookAndFeel de FlexStrip (ou de remplacer ce fichier par le sien).
// =============================================================================
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace PlexisTheme
{
    using juce::Colour;
    inline const Colour bgTop        { 0xff26282c };
    inline const Colour bgBottom     { 0xff141517 };
    inline const Colour panel        { 0xff2b2e33 };
    inline const Colour panelEdge    { 0xff3d4147 };
    inline const Colour panelShadow  { 0xff0b0c0d };
    inline const Colour headerStrip  { 0xff1b1c1f };
    inline const Colour accent       { 0xfff0a030 };   // ambre PLEXIS
    inline const Colour accentDim    { 0xff7a5420 };
    inline const Colour text         { 0xffdcd8d0 };
    inline const Colour textDim      { 0xff8d8a84 };
    inline const Colour knobLight    { 0xff55595f };
    inline const Colour knobDark     { 0xff1a1c1f };
    inline const Colour track        { 0xff101113 };
    inline const Colour ledOff       { 0xff3a3128 };
    inline const Colour ledOn        { 0xffffb444 };
    inline const Colour lcdBg        { 0xff0d0f10 };
    inline const Colour lcdText      { 0xffffb444 };

    inline juce::Font font (float size, bool bold = true)
    {
        return juce::Font (juce::FontOptions (size).withStyle (bold ? "Bold" : "Regular"));
    }
}

class PlexisLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PlexisLookAndFeel()
    {
        using namespace PlexisTheme;
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::ComboBox::backgroundColourId, lcdBg);
        setColour (juce::ComboBox::textColourId, lcdText);
        setColour (juce::ComboBox::outlineColourId, panelEdge);
        setColour (juce::ComboBox::arrowColourId, accent);
        setColour (juce::PopupMenu::backgroundColourId, Colour (0xff1c1d20));
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accentDim);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::Label::textColourId, text);
        setColour (juce::BubbleComponent::backgroundColourId, lcdBg);
        setColour (juce::BubbleComponent::outlineColourId, accent);
        setColour (juce::TooltipWindow::backgroundColourId, lcdBg);
        setColour (juce::TooltipWindow::textColourId, lcdText);
    }

    // ---- Potentiomètre rotatif : corps métallique, arc de valeur ambre ---------
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAng, float endAng, juce::Slider& s) override
    {
        using namespace PlexisTheme;
        const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
        const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float ang = startAng + pos * (endAng - startAng);

        // graduations
        g.setColour (textDim.withAlpha (0.55f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAng + (float) i / 10.0f * (endAng - startAng);
            const float r1 = radius * 0.93f, r2 = radius * (i % 5 == 0 ? 1.0f : 0.97f);
            g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), 1.0f);
        }

        // piste + arc de valeur (depuis le centre pour les paramètres bipolaires)
        const float arcR = radius * 0.84f;
        juce::Path trackPath; trackPath.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAng, endAng, true);
        g.setColour (track);
        g.strokePath (trackPath, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const bool bipolar = s.getMinimum() < 0.0 && s.getMaximum() > 0.0;
        const float from = bipolar ? (startAng + endAng) * 0.5f : startAng;
        juce::Path arc; arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
        g.setColour (s.isEnabled() ? accent : accentDim);
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // ombre portée et corps
        const float bodyR = radius * 0.68f;
        g.setColour (panelShadow.withAlpha (0.6f));
        g.fillEllipse (c.x - bodyR + 1.5f, c.y - bodyR + 3.0f, bodyR * 2, bodyR * 2);
        juce::ColourGradient grad (knobLight, c.x - bodyR * 0.6f, c.y - bodyR * 0.8f, knobDark, c.x + bodyR * 0.5f, c.y + bodyR, true);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - bodyR, c.y - bodyR, bodyR * 2, bodyR * 2);
        // jupe moletée
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        for (int i = 0; i < 24; ++i)
        {
            const float a = (float) i / 24.0f * juce::MathConstants<float>::twoPi;
            g.drawLine (c.x + bodyR * 0.86f * std::sin (a), c.y - bodyR * 0.86f * std::cos (a),
                        c.x + bodyR * std::sin (a), c.y - bodyR * std::cos (a), 1.0f);
        }
        // calotte
        const float capR = bodyR * 0.74f;
        juce::ColourGradient cap (Colour (0xff6a6e75), c.x, c.y - capR, Colour (0xff25272b), c.x, c.y + capR, false);
        g.setGradientFill (cap);
        g.fillEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (c.x - capR, c.y - capR, capR * 2, capR * 2, 1.0f);

        // index
        juce::Path ptr; ptr.addRoundedRectangle (-1.6f, -bodyR * 0.95f, 3.2f, bodyR * 0.5f, 1.5f);
        g.setColour (text);
        g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }

    // ---- Fader d'enveloppe -------------------------------------------------------
    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float, float,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        using namespace PlexisTheme;
        if (style != juce::Slider::LinearVertical) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, 0, 0, style, s); return; }
        const float cx = (float) x + (float) w * 0.5f;
        auto slot = juce::Rectangle<float> (cx - 3.0f, (float) y + 4.0f, 6.0f, (float) h - 8.0f);
        g.setColour (track); g.fillRoundedRectangle (slot, 3.0f);
        g.setColour (accent.withAlpha (0.85f));
        g.fillRoundedRectangle (slot.withTop (sliderPos), 3.0f);
        // graduations
        g.setColour (textDim.withAlpha (0.4f));
        for (int i = 0; i <= 10; ++i)
        {
            const float yy = slot.getY() + slot.getHeight() * (float) i / 10.0f;
            g.drawHorizontalLine ((int) yy, (float) x + 2.0f, cx - 7.0f);
            g.drawHorizontalLine ((int) yy, cx + 7.0f, (float) (x + w) - 2.0f);
        }
        // capuchon
        auto capR = juce::Rectangle<float> ((float) w - 6.0f, 22.0f).withCentre ({ cx, sliderPos });
        g.setColour (panelShadow.withAlpha (0.6f)); g.fillRoundedRectangle (capR.translated (0, 2), 3.0f);
        juce::ColourGradient grad (Colour (0xff70747b), capR.getX(), capR.getY(), Colour (0xff2a2c30), capR.getX(), capR.getBottom(), false);
        g.setGradientFill (grad); g.fillRoundedRectangle (capR, 3.0f);
        g.setColour (text); g.fillRect (capR.withSizeKeepingCentre (capR.getWidth() - 6.0f, 2.0f));
    }

    // ---- Interrupteur à DEL ------------------------------------------------------
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        using namespace PlexisTheme;
        auto r = b.getLocalBounds().toFloat();
        auto led = juce::Rectangle<float> (2.0f, r.getCentreY() - 6.0f, 22.0f, 12.0f);
        drawLed (g, led, b.getToggleState(), hover);
        g.setColour (b.getToggleState() ? text : textDim);
        g.setFont (font (11.5f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (30.0f), juce::Justification::centredLeft);
    }

    static void drawLed (juce::Graphics& g, juce::Rectangle<float> led, bool on, bool hover)
    {
        using namespace PlexisTheme;
        if (on)
        {
            g.setColour (ledOn.withAlpha (0.25f));
            g.fillRoundedRectangle (led.expanded (3.0f), 4.0f);
            juce::ColourGradient lg (Colour (0xfffff0c0), led.getCentreX(), led.getY(), ledOn, led.getCentreX(), led.getBottom(), false);
            g.setGradientFill (lg);
        }
        else g.setColour (hover ? ledOff.brighter (0.25f) : ledOff);
        g.fillRoundedRectangle (led, 2.5f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (led, 2.5f, 1.0f);
    }

    // ---- Afficheur LCD (listes déroulantes) ----------------------------------------
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox&) override
    {
        using namespace PlexisTheme;
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
        g.setColour (lcdBg); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (panelEdge); g.drawRoundedRectangle (r, 3.0f, 1.0f);
        juce::Path tri; const float ax = (float) w - 10.0f, ay = (float) h * 0.5f;
        tri.addTriangle (ax - 3.5f, ay - 2.0f, ax + 3.5f, ay - 2.0f, ax, ay + 2.5f);
        g.setColour (accent.withAlpha (0.8f)); g.fillPath (tri);
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return PlexisTheme::font (12.5f); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (2, 1, box.getWidth() - 16, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centred);
    }
    juce::Font getPopupMenuFont() override { return PlexisTheme::font (13.0f, false); }

    // ---- Bulle de valeur pendant la manipulation -------------------------------------
    juce::Font getSliderPopupFont (juce::Slider&) override { return PlexisTheme::font (12.5f); }
    void drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&, const juce::Rectangle<float>& body) override
    {
        using namespace PlexisTheme;
        g.setColour (lcdBg.withAlpha (0.95f)); g.fillRoundedRectangle (body, 3.0f);
        g.setColour (accent); g.drawRoundedRectangle (body.reduced (0.5f), 3.0f, 1.0f);
    }
};
