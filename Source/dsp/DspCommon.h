// =============================================================================
//  PLEXIS Labs — Charlie
//  DspCommon.h : constantes et utilitaires partagés (C++ pur, aucune dépendance JUCE)
//  Le moteur est volontairement indépendant de JUCE pour être testable seul et
//  réutilisable pour les futures déclinaisons (3 osc type Model D, 4 osc type Mono/Poly).
// =============================================================================
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace plexis
{
constexpr float kPi    = 3.14159265358979323846f;
constexpr float kTwoPi = 6.28318530717958647692f;
constexpr double kPiD  = 3.14159265358979323846; // M_PI n'est pas standard (MSVC)

// ---- Dimensions de l'architecture -----------------------------------------
// Pour passer au modèle 3 ou 4 oscillateurs, on augmente kNumOscillators :
// les boucles de voix, le mélangeur et le routage de modulation sont indexés.
constexpr int kNumOscillators = 2;
constexpr int kMaxPolyphony   = 8;
constexpr int kMaxUnison      = 7;

inline float clampf (float x, float lo, float hi) { return std::min (hi, std::max (lo, x)); }
inline float lerp   (float a, float b, float t)   { return a + (b - a) * t; }

inline float midiToHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }

// Saturation douce rapide (approximation de tanh, Padé 3/2, bornée)
inline float fastTanh (float x)
{
    x = clampf (x, -3.0f, 3.0f);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Générateur pseudo-aléatoire léger (xorshift32), sortie dans [-1, 1]
struct Rng
{
    uint32_t state = 0x12345678u;
    explicit Rng (uint32_t seed = 0x12345678u) : state (seed ? seed : 1u) {}
    inline uint32_t next() { state ^= state << 13; state ^= state >> 17; state ^= state << 5; return state; }
    inline float bipolar() { return (float) (int32_t) next() * (1.0f / 2147483648.0f); }
    inline float unipolar() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }
};

// Coefficient d'un lissage exponentiel pour une constante de temps en secondes
inline float onePoleCoef (float timeSec, float sampleRate)
{
    if (timeSec <= 0.0f) return 1.0f;
    return 1.0f - std::exp (-1.0f / (timeSec * sampleRate));
}
} // namespace plexis
