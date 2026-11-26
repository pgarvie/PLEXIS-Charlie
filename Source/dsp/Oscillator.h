// =============================================================================
//  PLEXIS Labs — Charlie
//  Oscillator.h : oscillateur anti-repliement (PolyBLEP) à forme modulable
//    Sine     : SHAPE = auto-modulation de phase (sinus -> timbre brillant)
//    Triangle : SHAPE = repliement (wavefolding) progressif
//    Saw      : SHAPE = ajout d'une 2e dent de scie déphasée (vers octave sup.)
//    Pulse    : SHAPE = largeur d'impulsion 50 % -> 5 %
// =============================================================================
#pragma once
#include "DspCommon.h"

namespace plexis
{
enum class Waveform : int { Sine = 0, Triangle, Saw, Pulse, NumWaves };

class Oscillator
{
public:
    void reset (double startPhase = 0.0) { phase = startPhase - std::floor (startPhase); }
    double getPhase() const { return phase; }

    // Remise à zéro (synchro dure) à une position fractionnaire dans l'échantillon
    void hardSync (double fractionAfterWrap, double incr)
    {
        phase = fractionAfterWrap * incr;
    }

    // Calcule un échantillon et avance la phase.
    // 'wrapped' indique si la phase a bouclé ; 'wrapFrac' = fraction de l'échantillon
    // écoulée après le bouclage (utile pour la synchro dure de l'oscillateur esclave).
    inline float process (Waveform w, float shape, double incr, bool& wrapped, double& wrapFrac)
    {
        const double dt = std::min (incr, 0.5);
        const float  p  = (float) phase;
        float out = 0.0f;

        switch (w)
        {
            case Waveform::Sine:
            {
                const float pm = shape * 1.6f * std::sin (kTwoPi * p);
                out = std::sin (kTwoPi * p + pm);
                break;
            }
            case Waveform::Triangle:
            {
                float tri = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
                float x = tri * (1.0f + 3.0f * shape);
                // repliement triangulaire dans [-1, 1]
                x = 0.25f * x + 0.25f;
                x = x - std::floor (x);
                out = (x < 0.5f) ? (4.0f * x - 1.0f) : (3.0f - 4.0f * x);
                break;
            }
            case Waveform::Saw:
            {
                const float s1 = saw (phase, dt);
                double p2 = phase + 0.5; if (p2 >= 1.0) p2 -= 1.0;
                const float s2 = saw (p2, dt);
                out = (s1 + shape * s2) / (1.0f + 0.6f * shape);
                break;
            }
            case Waveform::Pulse:
            {
                const double pw = 0.5 - 0.45 * (double) shape;
                double p2 = phase + (1.0 - pw); if (p2 >= 1.0) p2 -= 1.0;
                // impulsion = différence de deux dents de scie (moyenne nulle par construction)
                out = (saw (phase, dt) - saw (p2, dt)) * 0.9f;
                break;
            }
            default: break;
        }

        phase += incr;
        wrapped = false;
        if (phase >= 1.0)
        {
            phase -= 1.0;
            wrapped  = true;
            wrapFrac = (incr > 0.0) ? phase / incr : 0.0;
        }
        return out;
    }

private:
    static inline float polyBlep (double t, double dt)
    {
        if (t < dt)       { t /= dt;         return (float) (t + t - t * t - 1.0); }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
        return 0.0f;
    }
    static inline float saw (double p, double dt) { return (float) (2.0 * p - 1.0) - polyBlep (p, dt); }

    double phase = 0.0;
};
} // namespace plexis
