// =============================================================================
//  PLEXIS Labs — Charlie
//  PluginEditor.h
// =============================================================================
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "gui/Controls.h"

class CharlieEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit CharlieEditor (CharlieProcessor&);
    ~CharlieEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kWidth = 1000, kHeight = 590;

private:
    void timerCallback() override;
    void refreshPresetBox();

    struct Section { juce::String title; juce::Rectangle<int> bounds; };
    juce::Rectangle<int> sec (const juce::String& title, int col, int y, int h);
    void place (juce::Component& c, const juce::Rectangle<int>& s, int x, int y, int w, int h);

    CharlieProcessor& proc;
    PlexisLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };
    std::vector<Section> sections;
    std::vector<std::unique_ptr<juce::Component>> owned;

    template <typename T, typename... Args>
    T& make (Args&&... args)
    {
        auto c = std::make_unique<T> (std::forward<Args> (args)...);
        T& ref = *c; addAndMakeVisible (ref); owned.push_back (std::move (c)); return ref;
    }

    // contrôles nommés (pour le placement)
    IconSelector *osc1Wave{}, *osc2Wave{}, *filterType{}, *lfoWave{};
    Knob *osc1Vol{}, *shape{}, *osc2Vol{}, *osc2Tune{}, *pTune{}, *pMod{}, *pDetune{}, *pWheel{};
    Knob *nShape{}, *nVol{}, *cutoff{}, *reso{}, *fMod{}, *kbd{}, *uDetune{}, *lDelay{}, *lRate{};
    Knob *glide{}, *spread{}, *master{};
    RadioLeds *shapeMod{}, *pSrc{}, *fSrc{}, *vMode{};
    LedToggle *ring{}, *sync{}, *pInv{}, *pOsc1{}, *pOsc2{}, *fInv{}, *mwhl{}, *dco{};
    LcdChoice *octave{}, *voices{}, *lPhase{}, *lSync{};
    EnvFader* ampF[5] {}; EnvFader* modF[5] {};

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::MidiKeyboardComponent keyboard;
    int lastVoiceCount = -1, lastProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CharlieEditor)
};
