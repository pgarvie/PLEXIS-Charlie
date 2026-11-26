// =============================================================================
//  PLEXIS Labs — Charlie
//  Envelope.h : ADSR de type circuit RC (courbes exponentielles analogiques)
//    L'attaque vise 1.3 et s'arrête à 1.0 -> pente "punchy" d'un vrai condensateur.
// =============================================================================
#pragma once
#include "DspCommon.h"

namespace plexis
{
struct AdsrSettings
{
    float attack = 0.005f, decay = 0.3f, sustain = 0.7f, release = 0.3f; // secondes / niveau
};

class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate (float sr) { sampleRate = sr; }
    void setSettings (const AdsrSettings& s)
    {
        settings = s;
        aCoef = onePoleCoef (std::max (0.0005f, s.attack) * 0.83f, sampleRate);
        dCoef = onePoleCoef (std::max (0.001f,  s.decay)  * 0.35f, sampleRate);
        rCoef = onePoleCoef (std::max (0.001f,  s.release)* 0.35f, sampleRate);
    }

    // retrigger=false : legato (l'enveloppe continue depuis son niveau courant)
    void noteOn (bool retrigger = true)
    {
        if (retrigger || stage == Stage::Idle || stage == Stage::Release)
            stage = Stage::Attack;
    }
    void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }
    void kill()    { stage = Stage::Idle; value = 0.0f; }

    bool isActive() const { return stage != Stage::Idle; }
    Stage getStage() const { return stage; }
    float getValue() const { return value; }

    inline float process()
    {
        switch (stage)
        {
            case Stage::Idle: value = 0.0f; break;
            case Stage::Attack:
                value += (1.3f - value) * aCoef;
                if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                value += (settings.sustain - value) * dCoef;
                if (std::abs (value - settings.sustain) < 1.0e-4f) { value = settings.sustain; stage = Stage::Sustain; }
                break;
            case Stage::Sustain:
                value += (settings.sustain - value) * dCoef;   // suit les changements de S en direct
                break;
            case Stage::Release:
                value += (0.0f - value) * rCoef;
                if (value < 1.0e-4f) { value = 0.0f; stage = Stage::Idle; }   // -80 dB : voix libérée
                break;
        }
        return value;
    }

private:
    AdsrSettings settings;
    Stage stage = Stage::Idle;
    float value = 0.0f, sampleRate = 44100.0f;
    float aCoef = 0.01f, dCoef = 0.001f, rCoef = 0.001f;
};
} // namespace plexis
