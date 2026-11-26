// =============================================================================
//  PLEXIS Labs — Charlie
//  Decimator.h : décimateur ×2 FIR demi-bande (fenêtre de Kaiser, 63 coefficients)
//  Les voix tournent à 2× la fréquence d'échantillonnage ; ce filtre ramène le
//  signal à la fréquence de l'hôte en éliminant les composantes repliées.
// =============================================================================
#pragma once
#include "DspCommon.h"
#include <vector>

namespace plexis
{
class HalfbandDecimator
{
public:
    static constexpr int kTaps = 63;

    HalfbandDecimator() { design(); }

    void reset() { std::fill (std::begin (hist), std::end (hist), 0.0f); pos = 0; }

    // in : 2*n échantillons suréchantillonnés -> out : n échantillons
    void process (const float* in, float* out, int n)
    {
        for (int m = 0; m < n; ++m)
        {
            push (in[2 * m]);
            push (in[2 * m + 1]);
            // convolution sur l'historique circulaire (les coefficients pairs ≠ centre sont nuls)
            float acc = h[kCenter] * at (kCenter);
            for (int k = 1; k <= kCenter; k += 2)
                acc += h[kCenter - k] * (at (kCenter - k) + at (kCenter + k));
            out[m] = acc;
        }
    }

    static constexpr int latencyBaseSamples() { return kCenter / 2; }

private:
    static constexpr int kCenter = (kTaps - 1) / 2;
    static constexpr int kHist   = 64; // puissance de 2 >= kTaps

    inline void  push (float x) { pos = (pos + 1) & (kHist - 1); hist[pos] = x; }
    inline float at (int k) const { return hist[(pos - k) & (kHist - 1)]; }

    static double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 30; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; }
        return sum;
    }

    void design()
    {
        const double beta = 8.0;
        double sum = 0.0;
        for (int i = 0; i < kTaps; ++i)
        {
            const int n = i - kCenter;
            const double sinc = (n == 0) ? 0.5 : std::sin (0.5 * kPiD * n) / (kPiD * n);
            const double r = (double) n / kCenter;
            const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / besselI0 (beta);
            h[i] = (float) (sinc * w);
            if (n != 0 && (n % 2) == 0) h[i] = 0.0f; // demi-bande exacte
            sum += h[i];
        }
        for (auto& c : h) c = (float) (c / sum);
    }

    float h[kTaps] {};
    float hist[kHist] {};
    int pos = 0;
};
} // namespace plexis
