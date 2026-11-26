// =============================================================================
//  PLEXIS Labs — Charlie
//  Parameters.h : identifiants, plages et valeurs par défaut de tous les paramètres
//  (automatisables dans le DAW), et conversion vers plexis::SynthParams.
// =============================================================================
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <cstring>
#include "dsp/SynthParams.h"

namespace ParamID
{
    // OSC 1 / OSC 2
    inline constexpr const char* osc1Wave = "osc1Wave";
    inline constexpr const char* osc1Vol  = "osc1Vol";
    inline constexpr const char* shape    = "shape";
    inline constexpr const char* shapeMod = "shapeMod";
    inline constexpr const char* osc2Wave = "osc2Wave";
    inline constexpr const char* osc2Vol  = "osc2Vol";
    inline constexpr const char* ring     = "ring";
    inline constexpr const char* oscSync  = "sync";
    inline constexpr const char* osc2Oct  = "osc2Oct";
    inline constexpr const char* osc2Tune = "osc2Tune";
    // PITCH
    inline constexpr const char* pitchSrc    = "pitchSrc";
    inline constexpr const char* pitchInv    = "pitchInv";
    inline constexpr const char* pitchTune   = "pitchTune";
    inline constexpr const char* pitchMod    = "pitchMod";
    inline constexpr const char* pitchDetune = "pitchDetune";
    inline constexpr const char* pitchOsc1   = "pitchOsc1";
    inline constexpr const char* pitchOsc2   = "pitchOsc2";
    inline constexpr const char* bendRange   = "bendRange";
    // NOISE
    inline constexpr const char* noiseColor = "noiseColor";
    inline constexpr const char* noiseVol   = "noiseVol";
    // FILTER
    inline constexpr const char* filterType = "filterType";
    inline constexpr const char* cutoff     = "cutoff";
    inline constexpr const char* reso       = "reso";
    inline constexpr const char* filterSrc  = "filterSrc";
    inline constexpr const char* filterInv  = "filterInv";
    inline constexpr const char* filterMod  = "filterMod";
    inline constexpr const char* keyTrack   = "keyTrack";
    // UNISON
    inline constexpr const char* uniVoices = "uniVoices";
    inline constexpr const char* uniDetune = "uniDetune";
    // LFO
    inline constexpr const char* lfoWave  = "lfoWave";
    inline constexpr const char* lfoMwhl  = "lfoMwhl";
    inline constexpr const char* lfoDelay = "lfoDelay";
    inline constexpr const char* lfoPhase = "lfoPhase";
    inline constexpr const char* lfoSync  = "lfoSync";
    inline constexpr const char* lfoRate  = "lfoRate";
    // GLOBAL
    inline constexpr const char* voiceMode = "voiceMode";
    inline constexpr const char* dco       = "dco";
    inline constexpr const char* glide     = "glide";
    inline constexpr const char* spread    = "spread";
    inline constexpr const char* master    = "master";
    // ENVELOPPES
    inline constexpr const char* ampA = "ampA", *ampD = "ampD", *ampS = "ampS", *ampR = "ampR", *ampVel = "ampVel";
    inline constexpr const char* modA = "modA", *modD = "modD", *modS = "modS", *modR = "modR", *modVel = "modVel";
}

namespace ParamChoices
{
    inline const juce::StringArray oscWaves   { "Sine", "Triangle", "Saw", "Pulse" };
    inline const juce::StringArray shapeMod   { "MAN", "ENV", "LFO" };
    inline const juce::StringArray modSrc     { "ENV", "LFO" };
    inline const juce::StringArray octave     { "-2", "-1", "0", "+1", "+2" };
    inline const juce::StringArray filterType { "LP 24", "LP 12", "BP" };
    inline const juce::StringArray voices     { "1", "2", "3", "4", "5", "6", "7" };
    inline const juce::StringArray lfoWaves   { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Random" };
    inline const juce::StringArray lfoPhase   { "Free", "0\xc2\xb0", "90\xc2\xb0", "180\xc2\xb0", "270\xc2\xb0" };
    inline const juce::StringArray lfoSync    { "Off", "4/1", "2/1", "1/1", "1/2", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
    inline const juce::StringArray voiceMode  { "RTRG", "LEGA", "POLY" };

    // durée en noires de chaque division de synchro (index 0 = Off)
    inline double syncBeats (int idx)
    {
        static const double beats[] = { 0.0, 16.0, 8.0, 4.0, 2.0, 1.0, 2.0 / 3.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
        return beats[juce::jlimit (0, 11, idx)];
    }
}

// Accès rapide (temps réel, sans allocation) aux valeurs brutes
struct ParamPointers
{
    static constexpr int kMax = 80;
    const char* names[kMax] {};
    std::atomic<float>* p[kMax] {};
    int count = 0;

    void add (juce::AudioProcessorValueTreeState& s, const char* id)
    {
        jassert (count < kMax);
        names[count] = id;
        p[count] = s.getRawParameterValue (id);
        jassert (p[count] != nullptr);
        ++count;
    }
    float get (const char* id) const
    {
        for (int i = 0; i < count; ++i)
            if (names[i] == id || std::strcmp (names[i], id) == 0) return p[i]->load();
        jassertfalse;
        return 0.0f;
    }
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
const std::vector<const char*>& allParameterIds();
void fillSynthParams (const ParamPointers& pp, plexis::SynthParams& sp, double bpm);
