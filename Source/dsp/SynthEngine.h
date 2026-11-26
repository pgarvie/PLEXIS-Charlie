// =============================================================================
//  PLEXIS Labs — Charlie
//  SynthEngine.h : allocation des voix (POLY 8 voix, MONO RTRG, MONO LEGATO),
//                  portamento, pédale de maintien, rendu stéréo.
// =============================================================================
#pragma once
#include "Voice.h"
#include "Decimator.h"
#include <array>
#include <vector>

namespace plexis
{
class SynthEngine
{
public:
    static constexpr int kOversample = 2;
    static constexpr int kMaxChunk   = 256;   // taille de sous-bloc à la fréquence de l'hôte

    void prepare (float sr)
    {
        sampleRate = sr;
        // les voix tournent à 2× la fréquence de l'hôte (anti-repliement osc + filtre saturé)
        for (int v = 0; v < kMaxPolyphony; ++v) voices[v].prepare (sr * kOversample, 0x9E3779B9u * (uint32_t) (v + 1));
        osL.assign (kMaxChunk * kOversample, 0.0f);
        osR.assign (kMaxChunk * kOversample, 0.0f);
        decL.reset(); decR.reset();
        allNotesOff (true);
    }

    static constexpr int getLatencySamples() { return HalfbandDecimator::latencyBaseSamples(); }

    void setParams (const SynthParams& p)
    {
        if (p.voiceMode != params.voiceMode) allNotesOff (false); // évite les notes bloquées au changement de mode
        params = p;
    }
    const SynthParams& getParams() const { return params; }

    // ---- MIDI --------------------------------------------------------------
    void noteOn (int note, float vel)
    {
        note = std::max (0, std::min (127, note));
        removeFromStack (note);
        if (stackSize < (int) noteStack.size()) noteStack[stackSize++] = { note, vel };
        sustained[note] = false;

        if (params.voiceMode == VoiceMode::Poly) polyNoteOn (note, vel);
        else                                     monoNoteOn (note, vel);
        lastPitch = (float) note;
    }

    void noteOff (int note)
    {
        note = std::max (0, std::min (127, note));
        if (sustainPedal) { sustained[note] = true; removeFromStack (note); if (params.voiceMode != VoiceMode::Poly) monoAfterRemove (note); return; }
        removeFromStack (note);

        if (params.voiceMode == VoiceMode::Poly)
        {
            for (auto& v : voices) if (v.isGateOn() && v.getNote() == note) v.release();
        }
        else monoAfterRemove (note);
    }

    void setSustain (bool down)
    {
        sustainPedal = down;
        if (! down)
        {
            for (int n = 0; n < 128; ++n)
                if (sustained[n])
                {
                    sustained[n] = false;
                    if (! isInStack (n))
                        for (auto& v : voices) if (v.isGateOn() && v.getNote() == n) v.release();
                }
        }
    }

    void allNotesOff (bool hardKill)
    {
        stackSize = 0;
        for (auto& s : sustained) s = false;
        for (auto& v : voices) { if (hardKill) v.kill(); else v.release(); }
    }

    void setPitchBend (float bipolar) { mods.pitchBend = clampf (bipolar, -1.0f, 1.0f); }
    void setModWheel  (float unipolar) { mods.modWheel = clampf (unipolar, 0.0f, 1.0f); }

    // ---- Rendu ---------------------------------------------------------------
    void render (float* outL, float* outR, int numSamples)
    {
        for (int start = 0; start < numSamples; start += kMaxChunk)
        {
            const int n  = std::min (kMaxChunk, numSamples - start);
            const int n2 = n * kOversample;
            std::fill (osL.begin(), osL.begin() + n2, 0.0f);
            std::fill (osR.begin(), osR.begin() + n2, 0.0f);
            for (auto& v : voices) v.render (osL.data(), osR.data(), n2, params, mods);
            decL.process (osL.data(), outL + start, n);
            decR.process (osR.data(), outR + start, n);
        }

        // LFO global (phase "libre" partagée au déclenchement)
        globalLfoPhase += (double) params.lfoRateHz / sampleRate * numSamples;
        globalLfoPhase -= std::floor (globalLfoPhase);

        const float g = params.master * 0.5f;
        for (int i = 0; i < numSamples; ++i) { outL[i] *= g; outR[i] *= g; }
    }

    int getNumActiveVoices() const
    {
        int n = 0; for (auto& v : voices) n += v.isActive() ? 1 : 0; return n;
    }

private:
    struct HeldNote { int note; float vel; };

    void polyNoteOn (int note, float vel)
    {
        const float from = (params.glide > 0.0005f && lastPitch >= 0.0f) ? lastPitch : -1.0f;

        // 1) même note déjà en jeu -> on la redéclenche
        for (auto& v : voices)
            if (v.isActive() && v.getNote() == note) { v.start (note, vel, from, true, ++stamp, params, globalLfoPhase); return; }

        // 2) voix libre
        for (auto& v : voices)
            if (! v.isActive()) { v.start (note, vel, from, true, ++stamp, params, globalLfoPhase); return; }

        // 3) vol de voix : la plus ancienne relâchée, sinon la plus ancienne
        Voice* best = nullptr;
        for (auto& v : voices) if (! v.isGateOn() && (! best || v.getAge() < best->getAge())) best = &v;
        if (! best) for (auto& v : voices) if (! best || v.getAge() < best->getAge()) best = &v;
        best->start (note, vel, from, true, ++stamp, params, globalLfoPhase);
    }

    void monoNoteOn (int note, float vel)
    {
        Voice& v = voices[0];
        const bool legato = (params.voiceMode == VoiceMode::MonoLegato);
        if (v.isGateOn())
        {
            // note chevauchante : RTRG redéclenche, LEGA glisse sans redéclencher
            v.glideTo (note, vel, ! legato, params.glide <= 0.0005f);
        }
        else
        {
            const float from = (params.glide > 0.0005f && v.isActive() && lastPitch >= 0.0f) ? lastPitch : -1.0f;
            v.start (note, vel, from, true, ++stamp, params, globalLfoPhase);
        }
    }

    void monoAfterRemove (int removedNote)
    {
        Voice& v = voices[0];
        if (! v.isGateOn() || v.getNote() != removedNote) return;
        if (stackSize > 0)
        {
            const HeldNote prev = noteStack[stackSize - 1];
            const bool legato = (params.voiceMode == VoiceMode::MonoLegato);
            v.glideTo (prev.note, prev.vel, ! legato, params.glide <= 0.0005f);
            lastPitch = (float) prev.note;
        }
        else if (! sustainPedal) v.release();
    }

    void removeFromStack (int note)
    {
        int w = 0;
        for (int r = 0; r < stackSize; ++r) if (noteStack[r].note != note) noteStack[w++] = noteStack[r];
        stackSize = w;
    }
    bool isInStack (int note) const
    {
        for (int r = 0; r < stackSize; ++r) if (noteStack[r].note == note) return true;
        return false;
    }

    float sampleRate = 44100.0f;
    SynthParams params;
    GlobalMods mods;
    std::array<Voice, kMaxPolyphony> voices;
    std::array<HeldNote, 128> noteStack {};
    int stackSize = 0;
    std::array<bool, 128> sustained {};
    bool sustainPedal = false;
    float lastPitch = -1.0f;
    uint64_t stamp = 0;
    double globalLfoPhase = 0.0;
    std::vector<float> osL, osR;
    HalfbandDecimator decL, decR;
};
} // namespace plexis
