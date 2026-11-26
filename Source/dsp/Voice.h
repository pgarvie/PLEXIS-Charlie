// =============================================================================
//  PLEXIS Labs — Charlie
//  Voice.h : une voix de synthèse = N oscillateurs × U unissons -> bruit -> filtre
//            stéréo -> VCA. Les oscillateurs sont indexés (kNumOscillators) pour que
//            l'extension à 3 ou 4 oscillateurs se fasse sans réécrire la voix.
// =============================================================================
#pragma once
#include "SynthParams.h"

namespace plexis
{
struct GlobalMods
{
    float pitchBend = 0.0f;   // -1..+1
    float modWheel  = 0.0f;   // 0..1
};

class Voice
{
public:
    void prepare (float sr, uint32_t seed)
    {
        sampleRate = sr;
        rng = Rng (seed);
        ampEnv.setSampleRate (sr);
        modEnv.setSampleRate (sr);
        smoothCoef = onePoleCoef (0.008f, sr);
        reset();
    }

    void reset()
    {
        ampEnv.kill(); modEnv.kill();
        filterL.reset(); filterR.reset();
        note = -1; gate = false;
        for (int u = 0; u < kMaxUnison; ++u)
            for (int o = 0; o < kNumOscillators; ++o) { driftRatio[u][o] = 1.0f; driftCurrent[u][o] = 0.0f; driftTarget[u][o] = 0.0f; }
        noiseLpL = noiseLpR = 0.0f;
        primed = false;
    }

    bool  isActive() const   { return ampEnv.isActive(); }
    bool  isGateOn() const   { return gate; }
    int   getNote() const    { return note; }
    uint64_t getAge() const  { return age; }
    float getLevel() const   { return ampEnv.getValue(); }

    // fromPitch < 0 : pas de portamento (départ direct sur la note)
    void start (int midiNote, float vel, float fromPitch, bool retrigger, uint64_t stamp,
                const SynthParams& p, double globalLfoPhase)
    {
        const bool wasActive = isActive();
        note = midiNote; velocity = vel; gate = true; age = stamp;
        targetPitch  = (float) midiNote;
        currentPitch = (fromPitch >= 0.0f) ? fromPitch : targetPitch;

        if (! wasActive)
        {
            // phases d'oscillateur aléatoires : épaisseur naturelle de l'unisson
            for (int u = 0; u < kMaxUnison; ++u)
                for (int o = 0; o < kNumOscillators; ++o)
                    osc[u][o].reset (p.dco && u == 0 ? 0.0 : (double) rng.unipolar());
            filterL.reset(); filterR.reset();
        }

        if (retrigger || ! wasActive)
        {
            const double lfoStart = (p.lfoPhase == 0) ? globalLfoPhase : 0.25 * (p.lfoPhase - 1);
            lfo.start (lfoStart, rng.next());
        }
        ampEnv.noteOn (retrigger);
        modEnv.noteOn (retrigger);

        if (! primed) { primeSmoothers (p); primed = true; }
    }

    // Changement de note sans redéclenchement (mono legato / retour de pile)
    void glideTo (int midiNote, float vel, bool retrigger, bool instant)
    {
        note = midiNote; targetPitch = (float) midiNote; velocity = vel; gate = true;
        if (instant) currentPitch = targetPitch;
        if (retrigger) { ampEnv.noteOn (true); modEnv.noteOn (true); }
    }

    void release() { gate = false; ampEnv.noteOff(); modEnv.noteOff(); }
    void kill()    { reset(); }

    // Ajoute la sortie de la voix aux tampons L/R
    void render (float* outL, float* outR, int numSamples, const SynthParams& p, const GlobalMods& gm)
    {
        if (! isActive()) return;

        ampEnv.setSettings (p.ampEnv);
        modEnv.setSettings (p.modEnv);

        const int   U       = std::max (1, std::min (kMaxUnison, p.unisonVoices));
        const float uniNorm = 1.0f / std::sqrt ((float) U);
        const float glideCoef = (p.glide > 0.0005f) ? onePoleCoef (p.glide * 0.4f, sampleRate) : 1.0f;
        const double lfoIncr  = (double) p.lfoRateHz / sampleRate;
        const float lfoDelaySamples = p.lfoDelay * sampleRate;
        const float lfoDepth = p.lfoModWheel ? gm.modWheel : 1.0f;
        const float ampVelGain = 1.0f - p.ampVel * (1.0f - velocity);
        const float modVelGain = 1.0f - p.modVel * (1.0f - velocity);
        const float pitchModRange = 36.0f * p.pitchModAmt * p.pitchModAmt * p.pitchModAmt * (p.pitchModInv ? -1.0f : 1.0f);
        const float filterModOct  = 8.0f * p.filterModAmt * (p.filterModInv ? -1.0f : 1.0f);
        const float bendSemis     = gm.pitchBend * p.bendRange;
        const float kbdOct        = p.keyTrack * ((float) note - 60.0f) / 12.0f;

        // ---- rapports d'unisson et panoramique (constants sur le bloc) --------
        float uniRatio[kMaxUnison], panL[kMaxUnison], panR[kMaxUnison];
        const float maxCents = 50.0f * p.unisonDetune * p.unisonDetune + 6.0f * p.unisonDetune;
        for (int u = 0; u < U; ++u)
        {
            const float pos = (U == 1) ? 0.0f : (2.0f * (float) u / (float) (U - 1) - 1.0f); // -1..+1
            uniRatio[u] = std::exp2 (pos * maxCents / 1200.0f);
            const float pan = 0.5f + 0.5f * pos * p.spread;     // 0..1
            panL[u] = std::cos (pan * 0.5f * kPi) * 1.41421356f;
            panR[u] = std::sin (pan * 0.5f * kPi) * 1.41421356f;
        }

        // ---- cibles de lissage ---------------------------------------------------
        const float tgtLogCut = std::log2 (std::max (20.0f, p.cutoff));
        const float tgtRes    = p.resonance;
        float tgtVol[kNumOscillators];
        for (int o = 0; o < kNumOscillators; ++o) tgtVol[o] = p.osc[o].volume * p.osc[o].volume;
        const float tgtNoise = p.noiseVolume * p.noiseVolume;

        // dérive analogique (désactivée en mode DCO)
        updateDrift (p.dco, U, numSamples);

        const float noiseCoef = onePoleCoef (1.0f / (kTwoPi * 600.0f), sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            // lissages
            sLogCut += (tgtLogCut - sLogCut) * smoothCoef;
            sRes    += (tgtRes - sRes) * smoothCoef;
            for (int o = 0; o < kNumOscillators; ++o) sVol[o] += (tgtVol[o] - sVol[o]) * smoothCoef;
            sNoise  += (tgtNoise - sNoise) * smoothCoef;

            currentPitch += (targetPitch - currentPitch) * glideCoef;

            const float lfoVal = lfo.process (p.lfoWave, lfoIncr, lfoDelaySamples) * lfoDepth;
            const float menv   = modEnv.process() * modVelGain;
            const float aenv   = ampEnv.process();

            // forme (SHAPE) : manuelle, par enveloppe ou par LFO
            float shape = p.shape;
            if      (p.shapeMod == ShapeMod::Env) shape *= menv;
            else if (p.shapeMod == ShapeMod::Lfo) shape *= 0.5f + 0.5f * lfoVal;
            shape = clampf (shape, 0.0f, 1.0f);

            const float pitchModSrc = (p.pitchModSrc == ModSource::Env) ? menv : lfoVal;
            const float pitchMod    = pitchModSrc * pitchModRange;

            // incréments de base de chaque oscillateur
            double baseIncr[kNumOscillators];
            const float common = currentPitch + bendSemis + p.masterTune * 0.01f;
            for (int o = 0; o < kNumOscillators; ++o)
            {
                float pitch = common + (float) (12 * p.osc[o].octave) + p.osc[o].tune;
                if (p.osc[o].pitchModTarget) pitch += pitchMod;
                // DETUNE : écart symétrique entre OSC1 (-) et les autres (+)
                pitch += (o == 0 ? -0.5f : 0.5f) * p.detune;
                baseIncr[o] = (double) midiToHz (pitch) / (double) sampleRate;
            }

            float sumL = 0.0f, sumR = 0.0f;
            for (int u = 0; u < U; ++u)
            {
                float oscOut[kNumOscillators];
                bool wrapped = false; double wrapFrac = 0.0;

                // OSC1 : maître
                const double inc0 = baseIncr[0] * uniRatio[u] * driftRatio[u][0];
                oscOut[0] = osc[u][0].process (p.osc[0].wave, shape, inc0, wrapped, wrapFrac);

                for (int o = 1; o < kNumOscillators; ++o)
                {
                    const double inc = baseIncr[o] * uniRatio[u] * driftRatio[u][o];
                    bool w2 = false; double f2 = 0.0;
                    if (o == 1 && p.sync && wrapped) osc[u][o].hardSync (wrapFrac, inc);
                    oscOut[o] = osc[u][o].process (p.osc[o].wave, shape, inc, w2, f2);
                }
                if (p.ring && kNumOscillators > 1) oscOut[1] *= oscOut[0];

                float mix = 0.0f;
                for (int o = 0; o < kNumOscillators; ++o) mix += oscOut[o] * sVol[o];
                sumL += mix * panL[u];
                sumR += mix * panR[u];
            }
            sumL *= uniNorm; sumR *= uniNorm;

            // bruit stéréo coloré
            if (sNoise > 1.0e-6f)
            {
                const float wl = rng.bipolar();
                const float wr = lerp (wl, rng.bipolar(), p.spread);
                noiseLpL += (wl - noiseLpL) * noiseCoef;
                noiseLpR += (wr - noiseLpR) * noiseCoef;
                float nl, nr;
                if (p.noiseColor < 0.5f) { const float t = p.noiseColor * 2.0f; nl = lerp (noiseLpL * 3.0f, wl, t); nr = lerp (noiseLpR * 3.0f, wr, t); }
                else                     { const float t = (p.noiseColor - 0.5f) * 2.0f; nl = lerp (wl, wl - noiseLpL, t); nr = lerp (wr, wr - noiseLpR, t); }
                sumL += nl * sNoise * 0.7f;
                sumR += nr * sNoise * 0.7f;
            }

            // filtre : coupure modulée (enveloppe/LFO + suivi de clavier)
            const float fmodSrc = (p.filterModSrc == ModSource::Env) ? menv : lfoVal;
            const float cutHz   = std::exp2 (clampf (sLogCut + fmodSrc * filterModOct + kbdOct, 3.3f, 14.6f));
            const float yl = filterL.process (p.filterType, sumL, cutHz, sRes, sampleRate);
            const float yr = filterR.process (p.filterType, sumR, cutHz, sRes, sampleRate);

            const float gain = aenv * ampVelGain;
            outL[i] += yl * gain;
            outR[i] += yr * gain;
        }
    }

private:
    void primeSmoothers (const SynthParams& p)
    {
        sLogCut = std::log2 (std::max (20.0f, p.cutoff));
        sRes = p.resonance;
        for (int o = 0; o < kNumOscillators; ++o) sVol[o] = p.osc[o].volume * p.osc[o].volume;
        sNoise = p.noiseVolume * p.noiseVolume;
    }

    void updateDrift (bool dco, int U, int numSamples)
    {
        driftCounter += numSamples;
        const bool newTarget = driftCounter > (int) (sampleRate * 0.25f);
        if (newTarget) driftCounter = 0;
        const float k = std::min (1.0f, (float) numSamples / (sampleRate * 0.4f));
        for (int u = 0; u < U; ++u)
            for (int o = 0; o < kNumOscillators; ++o)
            {
                if (dco) { driftCurrent[u][o] = 0.0f; driftRatio[u][o] = 1.0f; continue; }
                if (newTarget) driftTarget[u][o] = rng.bipolar() * 3.0f;   // ±3 cents
                driftCurrent[u][o] += (driftTarget[u][o] - driftCurrent[u][o]) * k;
                driftRatio[u][o] = std::exp2 (driftCurrent[u][o] / 1200.0f);
            }
    }

    float sampleRate = 44100.0f;
    Rng rng;
    int note = -1; float velocity = 1.0f; bool gate = false; uint64_t age = 0;
    float currentPitch = 60.0f, targetPitch = 60.0f;

    Oscillator osc[kMaxUnison][kNumOscillators];
    float driftRatio[kMaxUnison][kNumOscillators] {}, driftCurrent[kMaxUnison][kNumOscillators] {}, driftTarget[kMaxUnison][kNumOscillators] {};
    int driftCounter = 0;

    VoiceFilter filterL, filterR;
    Envelope ampEnv, modEnv;
    Lfo lfo;
    float noiseLpL = 0.0f, noiseLpR = 0.0f;

    float smoothCoef = 0.01f, sLogCut = 13.0f, sRes = 0.0f, sVol[kNumOscillators] {}, sNoise = 0.0f;
    bool primed = false;
};
} // namespace plexis
