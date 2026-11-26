// =============================================================================
//  PLEXIS Labs — Charlie
//  SynthParams.h : instantané des réglages, en unités physiques, lu par le moteur.
//  Le processeur JUCE remplit cette structure une fois par bloc audio.
// =============================================================================
#pragma once
#include "DspCommon.h"
#include "Oscillator.h"
#include "Filters.h"
#include "Envelope.h"
#include "Lfo.h"

namespace plexis
{
enum class ModSource  : int { Env = 0, Lfo };
enum class ShapeMod   : int { Manual = 0, Env, Lfo };
enum class VoiceMode  : int { MonoRetrig = 0, MonoLegato, Poly };

struct OscSettings
{
    Waveform wave   = Waveform::Saw;
    float    volume = 0.8f;    // 0..1
    int      octave = 0;       // -2..+2 (utilisé par OSC 2 ; OSC 1 à 0)
    float    tune   = 0.0f;    // demi-tons (OSC 2 : -12..+12)
    bool     pitchModTarget = true; // reçoit la modulation de hauteur (PITCH > OSC1/OSC2)
};

struct SynthParams
{
    // ---- Oscillateurs ------------------------------------------------------
    OscSettings osc[kNumOscillators];
    float    shape    = 0.0f;              // 0..1 (commun aux oscillateurs)
    ShapeMod shapeMod = ShapeMod::Manual;
    bool     ring     = false;             // OSC2 = OSC2 × OSC1
    bool     sync     = false;             // OSC2 synchronisé sur OSC1

    // ---- Hauteur (PITCH) -----------------------------------------------------
    ModSource pitchModSrc = ModSource::Env;
    bool      pitchModInv = false;
    float     pitchModAmt = 0.0f;          // 0..1 (courbe cubique -> 0..36 demi-tons)
    float     masterTune  = 0.0f;          // cents (-100..+100)
    float     detune      = 0.0f;          // 0..1 -> écart OSC1/OSC2 jusqu'à ±50 cents
    float     bendRange   = 2.0f;          // demi-tons

    // ---- Bruit ---------------------------------------------------------------
    float noiseColor  = 0.5f;              // 0 sombre, 0.5 blanc, 1 brillant
    float noiseVolume = 0.0f;

    // ---- Filtre --------------------------------------------------------------
    FilterType filterType = FilterType::LP24;
    float      cutoff     = 8000.0f;        // Hz
    float      resonance  = 0.0f;           // 0..1
    ModSource  filterModSrc = ModSource::Env;
    bool       filterModInv = false;
    float      filterModAmt = 0.0f;         // 0..1 -> 0..8 octaves
    float      keyTrack     = 0.0f;         // 0..1

    // ---- Unisson -------------------------------------------------------------
    int   unisonVoices = 1;                 // 1..7
    float unisonDetune = 0.2f;              // 0..1

    // ---- LFO -----------------------------------------------------------------
    LfoWave lfoWave = LfoWave::Sine;
    float   lfoRateHz = 4.0f;               // fréquence effective (après synchro tempo)
    float   lfoDelay  = 0.0f;               // secondes
    int     lfoPhase  = 0;                  // 0 = libre, 1..4 = 0°/90°/180°/270° au déclenchement
    bool    lfoModWheel = false;            // profondeur contrôlée par la molette

    // ---- Global --------------------------------------------------------------
    VoiceMode voiceMode = VoiceMode::Poly;
    bool      dco       = false;            // true = accord stable (pas de dérive analogique)
    float     glide     = 0.0f;             // secondes
    float     spread    = 0.5f;             // largeur stéréo 0..1
    float     master    = 0.5f;             // gain linéaire

    // ---- Enveloppes ------------------------------------------------------------
    AdsrSettings ampEnv, modEnv;
    float ampVel = 0.5f, modVel = 0.0f;     // sensibilité à la vélocité 0..1
};
} // namespace plexis
