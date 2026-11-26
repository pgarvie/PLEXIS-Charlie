// =============================================================================
//  PLEXIS Labs — Charlie
//  Lfo.h : LFO par voix avec délai (fondu d'entrée) et formes multiples
// =============================================================================
#pragma once
#include "DspCommon.h"

namespace plexis
{
enum class LfoWave : int { Sine = 0, Triangle, SawUp, SawDown, Square, SampleHold, SmoothRandom, NumWaves };

class Lfo
{
public:
    void start (double startPhase, uint32_t seed)
    {
        phase = startPhase - std::floor (startPhase);
        rng = Rng (seed);
        held = rng.bipolar(); target = rng.bipolar(); smooth = held;
        delayPos = 0.0f;
    }

    // incr = fréquence / sampleRate ; delaySamples = durée du fondu d'entrée
    inline float process (LfoWave w, double incr, float delaySamples)
    {
        const float p = (float) phase;
        float out = 0.0f;
        switch (w)
        {
            case LfoWave::Sine:         out = std::sin (kTwoPi * p); break;
            case LfoWave::Triangle:     out = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p); break;
            case LfoWave::SawUp:        out = 2.0f * p - 1.0f; break;
            case LfoWave::SawDown:      out = 1.0f - 2.0f * p; break;
            case LfoWave::Square:       out = (p < 0.5f) ? 1.0f : -1.0f; break;
            case LfoWave::SampleHold:   out = held; break;
            case LfoWave::SmoothRandom: smooth += (target - smooth) * (float) std::min (1.0, incr * 6.0); out = smooth; break;
            default: break;
        }

        phase += incr;
        if (phase >= 1.0)
        {
            phase -= std::floor (phase);
            held = rng.bipolar();
            target = rng.bipolar();
        }

        if (delaySamples > 1.0f && delayPos < delaySamples)
        {
            const float fade = delayPos / delaySamples;
            delayPos += 1.0f;
            return out * fade * fade;
        }
        return out;
    }

private:
    double phase = 0.0;
    Rng rng;
    float held = 0.0f, target = 0.0f, smooth = 0.0f, delayPos = 0.0f;
};
} // namespace plexis
