// =============================================================================
//  PLEXIS Labs — Charlie
//  Filters.h : filtres "analogiques" à topologie préservée (TPT / ZDF)
//    LP24 : échelle de Moog 4 pôles (24 dB/oct), rétroaction résolue sans délai,
//           saturation tanh à l'entrée de l'échelle -> chaleur et auto-oscillation
//    LP12 : SVF 2 pôles (12 dB/oct), saturation de l'intégrateur
//    BP   : passe-bande issu du même SVF
// =============================================================================
#pragma once
#include "DspCommon.h"

namespace plexis
{
enum class FilterType : int { LP24 = 0, LP12, BP, NumTypes };

class LadderFilter
{
public:
    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }

    // cutoffHz : fréquence de coupure ; res : 0..1 (≈1 = auto-oscillation)
    inline float process (float x, float cutoffHz, float res, float sampleRate)
    {
        const float fc = clampf (cutoffHz, 10.0f, 0.45f * sampleRate);
        const float g  = std::tan (kPi * fc / sampleRate);
        const float G  = g / (1.0f + g);
        const float k  = 4.1f * res;                    // >4 => auto-oscillation
        const float oneMinusG = 1.0f - G;

        // Contribution des états à la sortie (y4 = G^4 u + S)
        const float S = G * G * G * oneMinusG * s[0] + G * G * oneMinusG * s[1]
                      + G * oneMinusG * s[2] + oneMinusG * s[3];
        const float G4 = G * G * G * G;

        // Compensation du gain de basse quand la résonance monte (comme un Model D "chaud")
        const float in = x * (1.0f + 0.5f * k);
        float u = (in - k * S) / (1.0f + k * G4);
        u = fastTanh (u);                               // non-linéarité d'entrée

        float y = u;
        for (int i = 0; i < 4; ++i)
        {
            const float v = (y - s[i]) * G;
            const float out = v + s[i];
            s[i] = out + v;
            y = out;
        }
        return y;
    }

private:
    float s[4] { 0, 0, 0, 0 };
};

class SvFilter
{
public:
    void reset() { ic1 = ic2 = 0.0f; }

    // Retourne LP ou BP selon 'bandpass'
    inline float process (float x, float cutoffHz, float res, float sampleRate, bool bandpass)
    {
        const float fc = clampf (cutoffHz, 10.0f, 0.45f * sampleRate);
        const float g  = std::tan (kPi * fc / sampleRate);
        const float kd = 2.0f - 1.97f * clampf (res, 0.0f, 1.0f); // amortissement
        const float a1 = 1.0f / (1.0f + g * (g + kd));
        const float a2 = g * a1;
        const float a3 = g * a2;

        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = fastTanh (2.0f * v1 - ic1);               // saturation douce de l'état
        ic2 = 2.0f * v2 - ic2;

        return bandpass ? v1 * kd : v2;
    }

private:
    float ic1 = 0.0f, ic2 = 0.0f;
};

// Filtre complet d'une voix : sélection du type
class VoiceFilter
{
public:
    void reset() { ladder.reset(); svf.reset(); }
    inline float process (FilterType t, float x, float fc, float res, float sr)
    {
        switch (t)
        {
            case FilterType::LP24: return ladder.process (x, fc, res, sr);
            case FilterType::LP12: return svf.process (fastTanh (x * 0.8f) * 1.25f, fc, res, sr, false);
            case FilterType::BP:   return svf.process (fastTanh (x * 0.8f) * 1.25f, fc, res, sr, true);
            default:               return x;
        }
    }
private:
    LadderFilter ladder;
    SvFilter svf;
};
} // namespace plexis
