// =============================================================================
//  PLEXIS Labs — Charlie
//  Parameters.cpp
// =============================================================================
#include "Parameters.h"

using namespace juce;
using APF = AudioParameterFloat;
using APC = AudioParameterChoice;
using APB = AudioParameterBool;

namespace
{
    NormalisableRange<float> timeRange (float lo, float hi)
    {
        NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (std::sqrt (lo * hi) * 0.6f);
        return r;
    }

    std::unique_ptr<APF> pct (const char* id, const char* name, float def)
    {
        return std::make_unique<APF> (ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 1.0f), def,
                                      AudioParameterFloatAttributes().withStringFromValueFunction (
                                          [] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; }));
    }

    std::unique_ptr<APF> seconds (const char* id, const char* name, float lo, float hi, float def)
    {
        return std::make_unique<APF> (ParameterID { id, 1 }, name, timeRange (lo, hi), def,
                                      AudioParameterFloatAttributes().withStringFromValueFunction (
                                          [] (float v, int) { return v < 1.0f ? String (v * 1000.0f, v < 0.01f ? 1 : 0) + " ms"
                                                                              : String (v, 2) + " s"; }));
    }

    std::unique_ptr<APC> choice (const char* id, const char* name, const StringArray& c, int def)
    {
        return std::make_unique<APC> (ParameterID { id, 1 }, name, c, def);
    }

    std::unique_ptr<APB> toggle (const char* id, const char* name, bool def)
    {
        return std::make_unique<APB> (ParameterID { id, 1 }, name, def);
    }
}

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout l;
    namespace C = ParamChoices;

    // ---- OSC 1 ---------------------------------------------------------------
    l.add (choice (ParamID::osc1Wave, "Osc1 Wave", C::oscWaves, 2));
    l.add (pct    (ParamID::osc1Vol,  "Osc1 Volume", 0.8f));
    l.add (pct    (ParamID::shape,    "Shape", 0.0f));
    l.add (choice (ParamID::shapeMod, "Shape Mod", C::shapeMod, 0));

    // ---- OSC 2 ---------------------------------------------------------------
    l.add (choice (ParamID::osc2Wave, "Osc2 Wave", C::oscWaves, 2));
    l.add (pct    (ParamID::osc2Vol,  "Osc2 Volume", 0.6f));
    l.add (toggle (ParamID::ring,     "Ring Mod", false));
    l.add (toggle (ParamID::oscSync,     "Sync", false));
    l.add (choice (ParamID::osc2Oct,  "Osc2 Octave", C::octave, 2));
    l.add (std::make_unique<APF> (ParameterID { ParamID::osc2Tune, 1 }, "Osc2 Tune",
                                  NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
                                  AudioParameterFloatAttributes().withStringFromValueFunction (
                                      [] (float v, int) { return (v > 0 ? "+" : "") + String (v, 2) + " st"; })));

    // ---- PITCH ---------------------------------------------------------------
    l.add (choice (ParamID::pitchSrc, "Pitch Mod Source", C::modSrc, 0));
    l.add (toggle (ParamID::pitchInv, "Pitch Mod Invert", false));
    l.add (std::make_unique<APF> (ParameterID { ParamID::pitchTune, 1 }, "Master Tune",
                                  NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f,
                                  AudioParameterFloatAttributes().withStringFromValueFunction (
                                      [] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + " ct"; })));
    l.add (pct    (ParamID::pitchMod,    "Pitch Mod Amount", 0.0f));
    l.add (pct    (ParamID::pitchDetune, "Detune", 0.08f));
    l.add (toggle (ParamID::pitchOsc1,   "Pitch Mod Osc1", true));
    l.add (toggle (ParamID::pitchOsc2,   "Pitch Mod Osc2", true));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::bendRange, 1 }, "Bend Range", 0, 24, 2,
                                                AudioParameterIntAttributes().withStringFromValueFunction (
                                                    [] (int v, int) { return String (v) + " st"; })));

    // ---- NOISE ---------------------------------------------------------------
    l.add (pct (ParamID::noiseColor, "Noise Shape", 0.5f));
    l.add (pct (ParamID::noiseVol,   "Noise Volume", 0.0f));

    // ---- FILTER --------------------------------------------------------------
    l.add (choice (ParamID::filterType, "Filter Type", C::filterType, 0));
    {
        NormalisableRange<float> r (20.0f, 20000.0f);
        r.setSkewForCentre (630.0f);
        l.add (std::make_unique<APF> (ParameterID { ParamID::cutoff, 1 }, "Cutoff", r, 6000.0f,
                                      AudioParameterFloatAttributes().withStringFromValueFunction (
                                          [] (float v, int) { return v < 1000 ? String (roundToInt (v)) + " Hz" : String (v / 1000.0f, 2) + " kHz"; })));
    }
    l.add (pct    (ParamID::reso,      "Resonance", 0.15f));
    l.add (choice (ParamID::filterSrc, "Filter Mod Source", C::modSrc, 0));
    l.add (toggle (ParamID::filterInv, "Filter Mod Invert", false));
    l.add (pct    (ParamID::filterMod, "Filter Mod Amount", 0.25f));
    l.add (pct    (ParamID::keyTrack,  "Key Tracking", 0.5f));

    // ---- UNISON --------------------------------------------------------------
    l.add (choice (ParamID::uniVoices, "Unison Voices", C::voices, 0));
    l.add (pct    (ParamID::uniDetune, "Unison Detune", 0.25f));

    // ---- LFO -----------------------------------------------------------------
    l.add (choice (ParamID::lfoWave,  "LFO Wave", C::lfoWaves, 0));
    l.add (toggle (ParamID::lfoMwhl,  "LFO Mod Wheel", false));
    l.add (seconds(ParamID::lfoDelay, "LFO Delay", 0.0f, 5.0f, 0.0f));
    l.add (choice (ParamID::lfoPhase, "LFO Phase", C::lfoPhase, 0));
    l.add (choice (ParamID::lfoSync,  "LFO Sync", C::lfoSync, 0));
    {
        NormalisableRange<float> r (0.02f, 40.0f);
        r.setSkewForCentre (2.0f);
        l.add (std::make_unique<APF> (ParameterID { ParamID::lfoRate, 1 }, "LFO Rate", r, 4.0f,
                                      AudioParameterFloatAttributes().withStringFromValueFunction (
                                          [] (float v, int) { return String (v, v < 1 ? 2 : 1) + " Hz"; })));
    }

    // ---- GLOBAL --------------------------------------------------------------
    l.add (choice  (ParamID::voiceMode, "Voice Mode", C::voiceMode, 2));
    l.add (toggle  (ParamID::dco,       "DCO", false));
    l.add (seconds (ParamID::glide,     "Glide", 0.0f, 2.0f, 0.0f));
    l.add (pct     (ParamID::spread,    "Stereo Spread", 0.5f));
    l.add (std::make_unique<APF> (ParameterID { ParamID::master, 1 }, "Master",
                                  NormalisableRange<float> (-36.0f, 6.0f, 0.1f), -3.0f,
                                  AudioParameterFloatAttributes().withStringFromValueFunction (
                                      [] (float v, int) { return String (v, 1) + " dB"; })));

    // ---- ENVELOPPES ------------------------------------------------------------
    l.add (seconds (ParamID::ampA, "Amp Attack",  0.0005f, 10.0f, 0.003f));
    l.add (seconds (ParamID::ampD, "Amp Decay",   0.001f,  10.0f, 0.4f));
    l.add (pct     (ParamID::ampS, "Amp Sustain", 0.75f));
    l.add (seconds (ParamID::ampR, "Amp Release", 0.001f,  10.0f, 0.25f));
    l.add (pct     (ParamID::ampVel, "Amp Velocity", 0.4f));
    l.add (seconds (ParamID::modA, "Mod Attack",  0.0005f, 10.0f, 0.002f));
    l.add (seconds (ParamID::modD, "Mod Decay",   0.001f,  10.0f, 0.5f));
    l.add (pct     (ParamID::modS, "Mod Sustain", 0.2f));
    l.add (seconds (ParamID::modR, "Mod Release", 0.001f,  10.0f, 0.3f));
    l.add (pct     (ParamID::modVel, "Mod Velocity", 0.0f));
    return l;
}

const std::vector<const char*>& allParameterIds()
{
    using namespace ParamID;
    static const std::vector<const char*> ids {
        osc1Wave, osc1Vol, shape, shapeMod, osc2Wave, osc2Vol, ring, oscSync, osc2Oct, osc2Tune,
        pitchSrc, pitchInv, pitchTune, pitchMod, pitchDetune, pitchOsc1, pitchOsc2, bendRange,
        noiseColor, noiseVol,
        filterType, cutoff, reso, filterSrc, filterInv, filterMod, keyTrack,
        uniVoices, uniDetune,
        lfoWave, lfoMwhl, lfoDelay, lfoPhase, lfoSync, lfoRate,
        voiceMode, dco, glide, spread, master,
        ampA, ampD, ampS, ampR, ampVel, modA, modD, modS, modR, modVel };
    return ids;
}

void fillSynthParams (const ParamPointers& pp, plexis::SynthParams& sp, double bpm)
{
    using namespace plexis;
    auto i = [&] (const char* id) { return (int) std::lround (pp.get (id)); };
    auto f = [&] (const char* id) { return pp.get (id); };
    auto b = [&] (const char* id) { return pp.get (id) > 0.5f; };

    sp.osc[0].wave   = (Waveform) i (ParamID::osc1Wave);
    sp.osc[0].volume = f (ParamID::osc1Vol);
    sp.osc[0].octave = 0;
    sp.osc[0].tune   = 0.0f;
    sp.osc[0].pitchModTarget = b (ParamID::pitchOsc1);

    sp.osc[1].wave   = (Waveform) i (ParamID::osc2Wave);
    sp.osc[1].volume = f (ParamID::osc2Vol);
    sp.osc[1].octave = i (ParamID::osc2Oct) - 2;
    sp.osc[1].tune   = f (ParamID::osc2Tune);
    sp.osc[1].pitchModTarget = b (ParamID::pitchOsc2);

    sp.shape    = f (ParamID::shape);
    sp.shapeMod = (ShapeMod) i (ParamID::shapeMod);
    sp.ring     = b (ParamID::ring);
    sp.sync     = b (ParamID::oscSync);

    sp.pitchModSrc = (ModSource) i (ParamID::pitchSrc);
    sp.pitchModInv = b (ParamID::pitchInv);
    sp.pitchModAmt = f (ParamID::pitchMod);
    sp.masterTune  = f (ParamID::pitchTune);
    sp.detune      = f (ParamID::pitchDetune);
    sp.bendRange   = f (ParamID::bendRange);

    sp.noiseColor  = f (ParamID::noiseColor);
    sp.noiseVolume = f (ParamID::noiseVol);

    sp.filterType   = (FilterType) i (ParamID::filterType);
    sp.cutoff       = f (ParamID::cutoff);
    sp.resonance    = f (ParamID::reso);
    sp.filterModSrc = (ModSource) i (ParamID::filterSrc);
    sp.filterModInv = b (ParamID::filterInv);
    sp.filterModAmt = f (ParamID::filterMod);
    sp.keyTrack     = f (ParamID::keyTrack);

    sp.unisonVoices = i (ParamID::uniVoices) + 1;
    sp.unisonDetune = f (ParamID::uniDetune);

    sp.lfoWave     = (LfoWave) i (ParamID::lfoWave);
    sp.lfoModWheel = b (ParamID::lfoMwhl);
    sp.lfoDelay    = f (ParamID::lfoDelay);
    sp.lfoPhase    = i (ParamID::lfoPhase);
    const int syncIdx = i (ParamID::lfoSync);
    if (syncIdx > 0 && bpm > 0.0)
        sp.lfoRateHz = (float) ((bpm / 60.0) / ParamChoices::syncBeats (syncIdx));
    else
        sp.lfoRateHz = f (ParamID::lfoRate);

    sp.voiceMode = (VoiceMode) i (ParamID::voiceMode);
    sp.dco       = b (ParamID::dco);
    sp.glide     = f (ParamID::glide);
    sp.spread    = f (ParamID::spread);
    sp.master    = Decibels::decibelsToGain (f (ParamID::master), -36.0f);

    sp.ampEnv = { f (ParamID::ampA), f (ParamID::ampD), f (ParamID::ampS), f (ParamID::ampR) };
    sp.modEnv = { f (ParamID::modA), f (ParamID::modD), f (ParamID::modS), f (ParamID::modR) };
    sp.ampVel = f (ParamID::ampVel);
    sp.modVel = f (ParamID::modVel);
}
