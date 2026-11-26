// =============================================================================
//  PLEXIS Labs — Charlie
//  Presets.h : banque d'usine (valeurs en unités réelles ; tout paramètre non
//  mentionné revient à sa valeur par défaut). Index de choix : voir Parameters.h.
// =============================================================================
#pragma once
#include "Parameters.h"
#include <vector>
#include <utility>

struct FactoryPreset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values;
};

inline const std::vector<FactoryPreset>& getFactoryPresets()
{
    using namespace ParamID;
    static const std::vector<FactoryPreset> presets {
        { "Init", {} },

        { "Warm Bass", { { voiceMode, 0 }, { osc1Wave, 2 }, { osc2Wave, 3 }, { osc2Oct, 1 }, { osc2Vol, 0.7f },
                         { cutoff, 380 }, { reso, 0.3f }, { filterMod, 0.42f }, { keyTrack, 0.3f },
                         { modD, 0.28f }, { modS, 0.0f }, { ampA, 0.001f }, { ampD, 0.6f }, { ampS, 0.8f }, { ampR, 0.08f },
                         { glide, 0.03f } } },

        { "Fat Unison Lead", { { voiceMode, 1 }, { osc1Wave, 2 }, { osc2Wave, 2 }, { uniVoices, 4 }, { uniDetune, 0.35f },
                               { cutoff, 2400 }, { reso, 0.25f }, { filterMod, 0.3f }, { glide, 0.08f },
                               { pitchSrc, 1 }, { pitchMod, 0.2f }, { pitchOsc1, 1 }, { pitchOsc2, 1 },
                               { lfoRate, 5.5f }, { lfoMwhl, 1 }, { lfoDelay, 0.3f }, { spread, 0.8f } } },

        { "Analog Pad", { { osc1Wave, 2 }, { osc2Wave, 2 }, { pitchDetune, 0.2f }, { uniVoices, 2 }, { uniDetune, 0.2f },
                          { cutoff, 1100 }, { reso, 0.2f }, { filterMod, 0.25f },
                          { modA, 1.2f }, { modD, 2.0f }, { modS, 0.5f }, { modR, 2.0f },
                          { ampA, 0.8f }, { ampD, 1.0f }, { ampS, 0.9f }, { ampR, 1.8f }, { ampVel, 0.2f },
                          { spread, 0.9f }, { shapeMod, 2 }, { shape, 0.6f }, { lfoRate, 0.3f }, { lfoWave, 1 } } },

        { "Sync Lead", { { voiceMode, 1 }, { osc1Wave, 2 }, { osc2Wave, 2 }, { ParamID::oscSync, 1 }, { osc1Vol, 0.3f }, { osc2Vol, 0.85f },
                         { pitchSrc, 0 }, { pitchOsc1, 0 }, { pitchOsc2, 1 }, { pitchMod, 0.55f },
                         { modD, 0.35f }, { modS, 0.0f }, { cutoff, 5000 }, { filterMod, 0.1f }, { glide, 0.05f } } },

        { "Brass", { { osc1Wave, 2 }, { osc2Wave, 2 }, { pitchDetune, 0.15f }, { cutoff, 650 }, { reso, 0.1f }, { filterMod, 0.45f },
                     { modA, 0.06f }, { modD, 0.6f }, { modS, 0.45f }, { ampA, 0.04f }, { ampS, 0.85f }, { ampR, 0.25f } } },

        { "Pluck", { { osc1Wave, 3 }, { osc2Wave, 2 }, { shape, 0.3f }, { cutoff, 300 }, { filterMod, 0.6f },
                     { modD, 0.25f }, { modS, 0.0f }, { ampD, 0.6f }, { ampS, 0.0f }, { ampR, 0.3f }, { ampVel, 0.7f }, { modVel, 0.5f } } },

        { "Acid Line", { { voiceMode, 1 }, { osc1Wave, 2 }, { osc2Vol, 0.0f }, { cutoff, 240 }, { reso, 0.82f },
                         { filterMod, 0.55f }, { modD, 0.2f }, { modS, 0.0f }, { glide, 0.06f }, { ampR, 0.05f }, { dco, 1 } } },

        { "Supersaw", { { osc1Wave, 2 }, { osc2Wave, 2 }, { uniVoices, 6 }, { uniDetune, 0.45f }, { pitchDetune, 0.1f },
                        { cutoff, 9000 }, { reso, 0.05f }, { filterMod, 0.0f }, { spread, 1.0f }, { ampR, 0.5f } } },

        { "Ring Bells", { { osc1Wave, 0 }, { osc2Wave, 0 }, { ring, 1 }, { osc2Oct, 3 }, { osc2Tune, 3.86f },
                          { osc1Vol, 0.4f }, { osc2Vol, 0.8f }, { cutoff, 12000 }, { filterMod, 0.0f },
                          { ampA, 0.001f }, { ampD, 1.5f }, { ampS, 0.0f }, { ampR, 1.2f }, { spread, 0.7f } } },
    };
    return presets;
}
