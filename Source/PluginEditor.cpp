// =============================================================================
//  PLEXIS Labs — Charlie
//  PluginEditor.cpp : disposition inspirée de Charlatan 2 (5 colonnes),
//                     habillage PLEXIS Labs
// =============================================================================
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
    constexpr int kMarginX = 14, kColW = 186, kColGap = 10, kTop = 60;
    constexpr int kKnobW = 70, kKnobH = 76;
}

CharlieEditor::CharlieEditor (CharlieProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;
    using K = IconSelector::Kind;
    namespace C = ParamChoices;

    // OSC 1
    osc1Wave = &make<IconSelector> (s, ParamID::osc1Wave, K::OscWave);
    osc1Vol  = &make<Knob> (s, ParamID::osc1Vol, "VOLUME");
    shapeMod = &make<RadioLeds> (s, ParamID::shapeMod, C::shapeMod);
    shape    = &make<Knob> (s, ParamID::shape, "SHAPE");
    // OSC 2
    osc2Wave = &make<IconSelector> (s, ParamID::osc2Wave, K::OscWave);
    osc2Vol  = &make<Knob> (s, ParamID::osc2Vol, "VOLUME");
    ring     = &make<LedToggle> (s, ParamID::ring, "RING");
    sync     = &make<LedToggle> (s, ParamID::oscSync, "SYNC");
    octave   = &make<LcdChoice> (s, ParamID::osc2Oct);
    osc2Tune = &make<Knob> (s, ParamID::osc2Tune, "TUNE");
    // PITCH
    pSrc    = &make<RadioLeds> (s, ParamID::pitchSrc, C::modSrc);
    pInv    = &make<LedToggle> (s, ParamID::pitchInv, "INV");
    pTune   = &make<Knob> (s, ParamID::pitchTune, "TUNE");
    pMod    = &make<Knob> (s, ParamID::pitchMod, "MOD");
    pDetune = &make<Knob> (s, ParamID::pitchDetune, "DETUNE");
    pOsc1   = &make<LedToggle> (s, ParamID::pitchOsc1, "OSC1");
    pOsc2   = &make<LedToggle> (s, ParamID::pitchOsc2, "OSC2");
    pWheel  = &make<Knob> (s, ParamID::bendRange, "PWHEEL");
    // NOISE
    nShape = &make<Knob> (s, ParamID::noiseColor, "SHAPE");
    nVol   = &make<Knob> (s, ParamID::noiseVol, "VOLUME");
    // FILTER
    filterType = &make<IconSelector> (s, ParamID::filterType, K::FilterType);
    cutoff = &make<Knob> (s, ParamID::cutoff, "CUTOFF");
    fSrc   = &make<RadioLeds> (s, ParamID::filterSrc, C::modSrc);
    fInv   = &make<LedToggle> (s, ParamID::filterInv, "INV");
    reso   = &make<Knob> (s, ParamID::reso, "Q");
    fMod   = &make<Knob> (s, ParamID::filterMod, "MOD");
    kbd    = &make<Knob> (s, ParamID::keyTrack, "KBD");
    // UNISON
    voices  = &make<LcdChoice> (s, ParamID::uniVoices);
    uDetune = &make<Knob> (s, ParamID::uniDetune, "DETUNE");
    // LFO
    lfoWave = &make<IconSelector> (s, ParamID::lfoWave, K::LfoWave);
    mwhl    = &make<LedToggle> (s, ParamID::lfoMwhl, "MWHL");
    lDelay  = &make<Knob> (s, ParamID::lfoDelay, "DELAY");
    lPhase  = &make<LcdChoice> (s, ParamID::lfoPhase);
    lSync   = &make<LcdChoice> (s, ParamID::lfoSync);
    lRate   = &make<Knob> (s, ParamID::lfoRate, "RATE");
    // GLOBAL
    vMode  = &make<RadioLeds> (s, ParamID::voiceMode, C::voiceMode);
    dco    = &make<LedToggle> (s, ParamID::dco, "DCO");
    glide  = &make<Knob> (s, ParamID::glide, "GLIDE");
    spread = &make<Knob> (s, ParamID::spread, "SPREAD");
    master = &make<Knob> (s, ParamID::master, "MASTER");
    // ENVELOPPES
    const char* ampIds[] = { ParamID::ampA, ParamID::ampD, ParamID::ampS, ParamID::ampR, ParamID::ampVel };
    const char* modIds[] = { ParamID::modA, ParamID::modD, ParamID::modS, ParamID::modR, ParamID::modVel };
    const char* envLbl[] = { "A", "D", "S", "R", "VEL" };
    for (int i = 0; i < 5; ++i)
    {
        ampF[i] = &make<EnvFader> (s, ampIds[i], envLbl[i]);
        modF[i] = &make<EnvFader> (s, modIds[i], envLbl[i]);
    }

    // Presets
    addAndMakeVisible (presetBox);
    refreshPresetBox();
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0 && idx != proc.getCurrentProgram()) { proc.setCurrentProgram (idx); lastProgram = idx; }
    };
    for (auto* b : { &prevBtn, &nextBtn })
    {
        addAndMakeVisible (*b);
        b->setColour (juce::TextButton::buttonColourId, PlexisTheme::lcdBg);
        b->setColour (juce::TextButton::textColourOffId, PlexisTheme::accent);
    }
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); presetBox.setSelectedItemIndex ((proc.getCurrentProgram() + n - 1) % n); };
    nextBtn.onClick = [this] { const int n = proc.getNumPrograms(); presetBox.setSelectedItemIndex ((proc.getCurrentProgram() + 1) % n); };

    // Clavier
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe9e5dc));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1a1b1d));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff6d6a64));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, PlexisTheme::accent.withAlpha (0.75f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, PlexisTheme::accent.withAlpha (0.25f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.5f));
    keyboard.setLowestVisibleKey (36);
    keyboard.setKeyWidth (17.0f);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    setSize (kWidth, kHeight);
    startTimerHz (15);
}

CharlieEditor::~CharlieEditor() { setLookAndFeel (nullptr); }

void CharlieEditor::refreshPresetBox()
{
    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    lastProgram = proc.getCurrentProgram();
}

void CharlieEditor::timerCallback()
{
    const int v = proc.getActiveVoiceCount();
    if (v != lastVoiceCount) { lastVoiceCount = v; repaint (getWidth() - 200, 0, 200, 52); }
    if (proc.getCurrentProgram() != lastProgram) refreshPresetBox();
}

juce::Rectangle<int> CharlieEditor::sec (const juce::String& title, int col, int y, int h)
{
    const juce::Rectangle<int> r (kMarginX + col * (kColW + kColGap), y, kColW, h);
    sections.push_back ({ title, r });
    return r;
}

void CharlieEditor::place (juce::Component& c, const juce::Rectangle<int>& s, int x, int y, int w, int h)
{
    c.setBounds (s.getX() + x, s.getY() + y, w, h);
}

void CharlieEditor::resized()
{
    sections.clear();
    const int y2a = kTop + 216, y2b = kTop + 290;

    // ---- colonne 1 : OSC 1 / OSC 2 ----------------------------------------------
    auto o1 = sec ("OSC 1", 0, kTop, 206);
    place (*osc1Wave, o1, 16, 38, 64, 42);
    place (*osc1Vol,  o1, 104, 30, kKnobW, kKnobH);
    place (*shapeMod, o1, 14, 112, 72, 54);
    place (*shape,    o1, 104, 116, kKnobW, kKnobH);

    auto o2 = sec ("OSC 2", 0, y2a, 206);
    place (*osc2Wave, o2, 16, 38, 64, 42);
    place (*osc2Vol,  o2, 104, 30, kKnobW, kKnobH);
    place (*ring,     o2, 14, 108, 80, 18);
    place (*sync,     o2, 14, 128, 80, 18);
    place (*octave,   o2, 16, 156, 58, 20);
    place (*osc2Tune, o2, 104, 116, kKnobW, kKnobH);

    // ---- colonne 2 : PITCH / NOISE --------------------------------------------------
    auto pi = sec ("PITCH", 1, kTop, 280);
    place (*pSrc,    pi, 14, 32, 72, 36);
    place (*pInv,    pi, 14, 70, 72, 18);
    place (*pTune,   pi, 104, 30, kKnobW, kKnobH);
    place (*pMod,    pi, 14, 112, kKnobW, kKnobH);
    place (*pDetune, pi, 104, 112, kKnobW, kKnobH);
    place (*pOsc1,   pi, 14, 204, 80, 18);
    place (*pOsc2,   pi, 14, 224, 80, 18);
    place (*pWheel,  pi, 104, 194, kKnobW, kKnobH);

    auto no = sec ("NOISE", 1, y2b, 132);
    place (*nShape, no, 14, 36, kKnobW, kKnobH);
    place (*nVol,   no, 104, 36, kKnobW, kKnobH);

    // ---- colonne 3 : FILTER / UNISON ------------------------------------------------
    auto fi = sec ("FILTER", 2, kTop, 280);
    place (*filterType, fi, 16, 38, 64, 42);
    place (*cutoff,     fi, 104, 30, kKnobW, kKnobH);
    place (*fSrc,       fi, 14, 114, 72, 36);
    place (*fInv,       fi, 14, 152, 72, 18);
    place (*reso,       fi, 104, 112, kKnobW, kKnobH);
    place (*fMod,       fi, 14, 194, kKnobW, kKnobH);
    place (*kbd,        fi, 104, 194, kKnobW, kKnobH);

    auto un = sec ("UNISON", 2, y2b, 132);
    place (*voices,  un, 18, 58, 54, 22);
    place (*uDetune, un, 104, 36, kKnobW, kKnobH);

    // ---- colonne 4 : LFO / GLOBAL ---------------------------------------------------
    auto lf = sec ("LFO", 3, kTop, 206);
    place (*lfoWave, lf, 16, 34, 64, 42);
    place (*mwhl,    lf, 14, 82, 80, 18);
    place (*lDelay,  lf, 104, 30, kKnobW, kKnobH);
    place (*lPhase,  lf, 16, 114, 58, 20);
    place (*lSync,   lf, 16, 152, 58, 20);
    place (*lRate,   lf, 104, 116, kKnobW, kKnobH);

    auto gl = sec ("GLOBAL", 3, y2a, 206);
    place (*vMode,  gl, 14, 30, 72, 54);
    place (*dco,    gl, 14, 88, 72, 18);
    place (*glide,  gl, 104, 30, kKnobW, kKnobH);
    place (*spread, gl, 14, 116, kKnobW, kKnobH);
    place (*master, gl, 104, 116, kKnobW, kKnobH);

    // ---- colonne 5 : enveloppes -----------------------------------------------------
    auto ae = sec ("AMP ENV", 4, kTop, 206);
    auto me = sec ("MOD ENV", 4, y2a, 206);
    for (int i = 0; i < 5; ++i)
    {
        place (*ampF[i], ae, 10 + i * 34, 30, 32, 170);
        place (*modF[i], me, 10 + i * 34, 30, 32, 170);
    }

    // ---- en-tête et clavier ---------------------------------------------------------
    prevBtn.setBounds (380, 15, 24, 24);
    presetBox.setBounds (408, 15, 200, 24);
    nextBtn.setBounds (612, 15, 24, 24);
    keyboard.setBounds (kMarginX, kTop + 432, getWidth() - 2 * kMarginX, getHeight() - (kTop + 432) - 10);
}

void CharlieEditor::paint (juce::Graphics& g)
{
    using namespace PlexisTheme;
    // fond
    g.setGradientFill (juce::ColourGradient (bgTop, 0, 0, bgBottom, 0, (float) getHeight(), false));
    g.fillAll();
    // texture brossée discrète
    juce::Random rnd (42);
    for (int y = 0; y < getHeight(); y += 2)
    {
        g.setColour (juce::Colours::white.withAlpha (0.008f + 0.012f * rnd.nextFloat()));
        g.drawHorizontalLine (y, 0.0f, (float) getWidth());
    }

    // ---- en-tête ----------------------------------------------------------------
    auto header = getLocalBounds().removeFromTop (52);
    g.setColour (headerStrip); g.fillRect (header);
    g.setColour (accent); g.fillRect (header.removeFromBottom (2));
    g.setColour (accent);
    g.setFont (font (11.0f));
    g.drawText ("PLEXIS LABS", 18, 8, 200, 14, juce::Justification::left);
    g.setColour (text);
    g.setFont (font (26.0f));
    g.drawText ("CHARLIE", 16, 18, 200, 30, juce::Justification::left);
    g.setColour (textDim);
    g.setFont (font (11.0f, false));
    g.drawText ("2-OSC VIRTUAL ANALOG SYNTHESIZER", 140, 30, 230, 14, juce::Justification::left);
    g.drawText ("PRESET", 330, 15, 46, 24, juce::Justification::centredRight);

    // compteur de voix (DEL) + version
    const int voicesOn = juce::jmax (0, lastVoiceCount);
    g.setFont (font (10.5f));
    g.setColour (textDim);
    g.drawText ("VOICES", getWidth() - 214, 8, 60, 14, juce::Justification::left);
    for (int i = 0; i < 8; ++i)
        PlexisLookAndFeel::drawLed (g, { (float) (getWidth() - 214 + i * 16), 26.0f, 12.0f, 8.0f }, i < voicesOn, false);
    g.drawText ("v1.0.0", getWidth() - 74, 8, 60, 14, juce::Justification::right);

    // ---- sections -------------------------------------------------------------------
    for (auto& s : sections)
    {
        auto r = s.bounds.toFloat();
        g.setColour (panelShadow.withAlpha (0.7f)); g.fillRoundedRectangle (r.translated (0, 3), 5.0f);
        g.setColour (panel); g.fillRoundedRectangle (r, 5.0f);
        g.setColour (panelEdge); g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        auto titleBar = r.removeFromTop (22.0f);
        g.setColour (headerStrip); g.fillRoundedRectangle (titleBar.reduced (1.0f), 4.0f);
        g.setColour (accent); g.fillRect (titleBar.getX() + 8.0f, titleBar.getBottom() - 2.0f, 26.0f, 2.0f);
        g.setColour (accent);
        g.setFont (font (12.0f));
        g.drawText (s.title, titleBar.reduced (10.0f, 0.0f), juce::Justification::centredLeft);
    }

    // ---- étiquettes secondaires -----------------------------------------------------
    auto lbl = [&] (const juce::String& t, int col, int secY, int x, int y, int w)
    {
        g.setColour (text); g.setFont (font (11.0f));
        g.drawText (t, kMarginX + col * (kColW + kColGap) + x, secY + y, w, 14, juce::Justification::centred);
    };
    const int y2a = kTop + 216, y2b = kTop + 290;
    lbl ("MOD", 0, kTop, 14, 176, 60);
    lbl ("OCTAVE", 0, y2a, 14, 178, 62);
    lbl ("VOICES", 2, y2b, 14, 84, 62);
    lbl ("PHASE", 3, kTop, 14, 136, 62);
    lbl ("SYNC", 3, kTop, 14, 174, 62);

    // traits de liaison type Charlatan (MOD —— SHAPE, OCTAVE —— TUNE, SYNC —— RATE)
    g.setColour (textDim.withAlpha (0.6f));
    auto link = [&] (int col, int secY, int y)
    {
        const float x0 = (float) (kMarginX + col * (kColW + kColGap) + 78), x1 = x0 + 34.0f;
        g.drawLine (x0, (float) (secY + y), x1, (float) (secY + y), 1.0f);
    };
    link (0, kTop, 183); link (0, y2a, 185); link (3, kTop, 181);
}
