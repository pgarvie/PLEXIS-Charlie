// Tests du moteur DSP de Charlie avec données synthétiques (sans JUCE).
// g++ -O2 -std=c++17 -I../Source tests/dsp_tests.cpp -o dsp_tests && ./dsp_tests
#include "dsp/SynthEngine.h"
#include <cstdio>
#include <vector>
#include <complex>
#include <chrono>

using namespace plexis;
static int failures = 0;
#define CHECK(cond, ...) do { bool ok_ = (cond); std::printf ("[%s] ", ok_ ? " OK " : "FAIL"); std::printf (__VA_ARGS__); std::printf ("\n"); if (!ok_) ++failures; } while (0)

static const float SR = 48000.0f;

// Amplitude (dB) d'une fréquence donnée par DFT ponctuelle avec fenêtre de Hann
static double toneDb (const std::vector<float>& x, double freq)
{
    std::complex<double> acc = 0; double wsum = 0;
    const size_t N = x.size();
    for (size_t n = 0; n < N; ++n)
    {
        const double w = 0.5 - 0.5 * std::cos (2 * M_PI * n / (N - 1));
        acc += (double) x[n] * w * std::polar (1.0, -2 * M_PI * freq * n / SR);
        wsum += w;
    }
    return 20 * std::log10 (std::abs (acc) * 2 / wsum + 1e-12);
}

static std::vector<float> renderOsc (Waveform w, float shape, float hz, int N)
{
    Oscillator o; o.reset (0.0);
    std::vector<float> v (N); bool wr; double wf;
    for (int i = 0; i < N; ++i) v[i] = o.process (w, shape, hz / SR, wr, wf);
    return v;
}

static void testOscillators()
{
    std::puts ("\n== Oscillateurs ==");
    const int N = 1 << 15;
    // Fréquence : le fondamental doit dominer à 440 Hz
    for (int wi = 0; wi < 4; ++wi)
    {
        auto v = renderOsc ((Waveform) wi, 0.0f, 440.0f, N);
        const double f0 = toneDb (v, 440.0), off = toneDb (v, 452.0);
        double mean = 0; for (float s : v) mean += s; mean /= N;
        float peak = 0; for (float s : v) peak = std::max (peak, std::abs (s));
        CHECK (f0 - off > 30 && std::abs (mean) < 0.01 && peak < 1.3f,
               "onde %d : fondamental %.1f dB, hors-fréquence %.1f dB, DC %.4f, crête %.2f", wi, f0, off, mean, peak);
    }
    // Impulsion : pas de composante continue, quelle que soit la largeur
    for (float s : { 0.0f, 0.5f, 1.0f })
    {
        auto v = renderOsc (Waveform::Pulse, s, 220.0f, N);
        double mean = 0; for (float x : v) mean += x; mean /= N;
        CHECK (std::abs (mean) < 0.01, "impulsion shape=%.1f : DC = %.4f", s, mean);
    }
    // Anti-repliement : dent de scie à 4 987 Hz, on mesure un repli typique
    {
        const double f = 4987.0;
        auto v = renderOsc (Waveform::Saw, 0.0f, (float) f, N);
        const double fund = toneDb (v, f);
        // harmoniques 5..9 se replient sous Nyquist : on prend la pire raie repliée
        double worst = -300;
        for (int h = 5; h <= 12; ++h)
        {
            double fa = std::fmod (h * f, SR); if (fa > SR / 2) fa = SR - fa;
            if (fa < 15000) worst = std::max (worst, toneDb (v, fa) - fund);
        }
        CHECK (worst < -30, "osc seul (sans suréchantillonnage), scie 5 kHz : pire repli audible %.1f dB", worst);
        // comparaison avec une dent de scie naïve
        std::vector<float> nv (N); double ph = 0;
        for (int i = 0; i < N; ++i) { nv[i] = (float) (2 * ph - 1); ph += f / SR; if (ph >= 1) ph -= 1; }
        double worstN = -300;
        for (int h = 5; h <= 12; ++h) { double fa = std::fmod (h * f, SR); if (fa > SR / 2) fa = SR - fa; worstN = std::max (worstN, toneDb (nv, fa) - toneDb (nv, f)); }
        std::printf ("       (référence naïve : %.1f dB)\n", worstN);
    }
}

static double filterGainDb (FilterType t, float fc, float res, float testHz)
{
    VoiceFilter f; f.reset();
    const int N = 1 << 14; std::vector<float> y (N);
    for (int i = 0; i < N; ++i) y[i] = f.process (t, 0.05f * std::sin (2 * M_PI * testHz * i / SR), fc, res, SR);
    std::vector<float> tail (y.begin() + N / 2, y.end());
    return toneDb (tail, testHz) - 20 * std::log10 (0.05);
}

static void testFilters()
{
    std::puts ("\n== Filtres ==");
    const double lp24_4k = filterGainDb (FilterType::LP24, 500, 0.0f, 4000), lp24_8k = filterGainDb (FilterType::LP24, 500, 0.0f, 8000);
    const double lp24_pass = filterGainDb (FilterType::LP24, 500, 0.0f, 50);
    CHECK (std::abs ((lp24_4k - lp24_8k) - 24) < 3, "LP24 : pente %.1f dB/oct (attendu ~24), bande passante %.1f dB", lp24_4k - lp24_8k, lp24_pass);
    const double lp12_4k = filterGainDb (FilterType::LP12, 500, 0.0f, 4000), lp12_8k = filterGainDb (FilterType::LP12, 500, 0.0f, 8000);
    CHECK (std::abs ((lp12_4k - lp12_8k) - 12) < 2, "LP12 : pente %.1f dB/oct (attendu ~12)", lp12_4k - lp12_8k);
    const double bpC = filterGainDb (FilterType::BP, 1000, 0.5f, 1000), bpL = filterGainDb (FilterType::BP, 1000, 0.5f, 100), bpH = filterGainDb (FilterType::BP, 1000, 0.5f, 10000);
    CHECK (bpC > bpL + 12 && bpC > bpH + 12, "BP : centre %.1f dB, 100 Hz %.1f dB, 10 kHz %.1f dB", bpC, bpL, bpH);
    const double pk = filterGainDb (FilterType::LP24, 1000, 0.8f, 1000), flat = filterGainDb (FilterType::LP24, 1000, 0.0f, 1000);
    CHECK (pk > flat + 6, "LP24 : pic de résonance +%.1f dB à fc (res 0.8 vs 0)", pk - flat);

    // Auto-oscillation : impulsion puis silence, res = 1
    {
        LadderFilter f; float last = 0, peak = 0;
        for (int i = 0; i < (int) SR; ++i) { float y = f.process (i == 0 ? 1.0f : 0.0f, 800, 1.0f, SR); if (i > SR * 0.8) peak = std::max (peak, std::abs (y)); last = y; }
        CHECK (peak > 0.05f && std::isfinite (last), "LP24 auto-oscillation à res=1 : amplitude %.3f après 0.8 s", peak);
    }
    // Stabilité : bruit fort, coupure balayée vite, résonance max
    {
        VoiceFilter f; Rng r; bool finite = true; float peak = 0;
        for (int t = 0; t < 3; ++t)
            for (int i = 0; i < (int) SR; ++i)
            {
                const float fc = 20.0f * std::pow (1000.0f, 0.5f + 0.5f * std::sin (i * 0.002f));
                float y = f.process ((FilterType) t, 4.0f * r.bipolar(), fc, 1.0f, SR);
                finite &= std::isfinite (y); peak = std::max (peak, std::abs (y));
            }
        CHECK (finite && peak < 10, "Stabilité (bruit ×4, balayage, res max) : fini=%d, crête %.2f", finite, peak);
    }
}

static void testEnvelope()
{
    std::puts ("\n== Enveloppes ==");
    Envelope e; e.setSampleRate (SR); AdsrSettings s; s.attack = 0.010f; s.decay = 0.1f; s.sustain = 0.5f; s.release = 0.2f; e.setSettings (s);
    e.noteOn(); int n = 0; while (e.getStage() == Envelope::Stage::Attack && n < SR) { e.process(); ++n; }
    CHECK (std::abs (n / SR - 0.010) < 0.003, "Attaque 10 ms -> crête atteinte en %.1f ms", 1000.0 * n / SR);
    for (int i = 0; i < (int) SR; ++i) e.process();
    CHECK (std::abs (e.getValue() - 0.5f) < 0.01f, "Maintien : %.3f (attendu 0.5)", e.getValue());
    e.noteOff(); n = 0; while (e.getValue() > 0.5f * 0.1f && n < SR * 4) { e.process(); ++n; }
    CHECK (n / SR > 0.1 && n / SR < 0.4, "Relâchement 200 ms -> -20 dB en %.0f ms", 1000.0 * n / SR);
    n = 0; while (e.isActive() && n < SR * 10) { e.process(); ++n; }
    CHECK (! e.isActive(), "Enveloppe retourne au repos (%.2f s)", n / SR);
}

static SynthParams fatPatch()
{
    SynthParams p;
    p.osc[0].wave = Waveform::Saw; p.osc[1].wave = Waveform::Pulse; p.osc[1].octave = -1;
    p.unisonVoices = 7; p.unisonDetune = 0.5f; p.resonance = 0.9f; p.cutoff = 1200.0f;
    p.filterModAmt = 0.6f; p.modEnv.decay = 0.4f; p.modEnv.sustain = 0.2f; p.noiseVolume = 0.3f;
    p.master = 1.0f; p.ampEnv.release = 0.05f;
    return p;
}

static void testEngine()
{
    std::puts ("\n== Moteur / voix ==");
    SynthEngine eng; eng.prepare (SR);
    const int B = 256; std::vector<float> L (B), R (B);

    // Polyphonie et vol de voix
    SynthParams p = fatPatch(); eng.setParams (p);
    for (int n = 0; n < 9; ++n) eng.noteOn (48 + n, 1.0f);
    eng.render (L.data(), R.data(), B);
    CHECK (eng.getNumActiveVoices() == 8, "9 notes en POLY -> %d voix actives (max 8)", eng.getNumActiveVoices());

    // Niveaux et finitude dans le pire cas (8 voix × 7 unissons, res 0.9, bruit)
    float peak = 0; bool finite = true;
    for (int b = 0; b < 400; ++b) { eng.render (L.data(), R.data(), B); for (int i = 0; i < B; ++i) { finite &= std::isfinite (L[i]) && std::isfinite (R[i]); peak = std::max ({ peak, std::abs (L[i]), std::abs (R[i]) }); } }
    CHECK (finite && peak < 4.0f, "Pire cas 8×7 : fini=%d, crête %.2f (master à 100 %%)", finite, peak);

    // Stéréo : l'unisson doit élargir l'image
    double corrNum = 0, eL = 0, eR = 0;
    for (int b = 0; b < 50; ++b) { eng.render (L.data(), R.data(), B); for (int i = 0; i < B; ++i) { corrNum += L[i] * R[i]; eL += L[i] * L[i]; eR += R[i] * R[i]; } }
    const double corr = corrNum / std::sqrt (eL * eR + 1e-20);
    CHECK (corr < 0.95, "Unisson 7 + spread 0.5 : corrélation L/R = %.2f (< 1 = image stéréo)", corr);

    // Changement de mode : aucune note bloquée
    p.voiceMode = VoiceMode::MonoLegato; eng.setParams (p);
    for (int b = 0; b < 200; ++b) eng.render (L.data(), R.data(), B);
    CHECK (eng.getNumActiveVoices() == 0, "Passage POLY -> LEGA : %d voix encore actives après 1 s", eng.getNumActiveVoices());

    // Mono legato : une seule voix, retour à la note précédente
    eng.noteOn (60, 1.0f); eng.noteOn (64, 1.0f); eng.render (L.data(), R.data(), B);
    CHECK (eng.getNumActiveVoices() == 1, "LEGA : notes chevauchées -> %d voix", eng.getNumActiveVoices());
    eng.noteOff (64); eng.render (L.data(), R.data(), B);
    CHECK (eng.getNumActiveVoices() == 1, "LEGA : relâche 64, 60 tenue -> voix toujours active");
    eng.noteOff (60); for (int b = 0; b < 200; ++b) eng.render (L.data(), R.data(), B);
    CHECK (eng.getNumActiveVoices() == 0, "LEGA : tout relâché -> silence");

    // Pédale de maintien
    p.voiceMode = VoiceMode::Poly; eng.setParams (p);
    eng.setSustain (true); eng.noteOn (60, 1.0f); eng.noteOff (60);
    for (int b = 0; b < 100; ++b) eng.render (L.data(), R.data(), B);
    const int held = eng.getNumActiveVoices();
    eng.setSustain (false); for (int b = 0; b < 200; ++b) eng.render (L.data(), R.data(), B);
    CHECK (held == 1 && eng.getNumActiveVoices() == 0, "Pédale : tenue=%d, après relâchement=%d", held, eng.getNumActiveVoices());

    // Hauteur : sinus pur, filtre ouvert, DCO, une note A4 -> 440 Hz
    {
        SynthParams s; s.osc[0].wave = Waveform::Sine; s.osc[1].volume = 0.0f; s.cutoff = 18000; s.dco = true;
        s.ampEnv.sustain = 1.0f; SynthEngine e2; e2.prepare (SR); e2.setParams (s); e2.noteOn (69, 1.0f);
        std::vector<float> all; for (int b = 0; b < 128; ++b) { e2.render (L.data(), R.data(), B); all.insert (all.end(), L.begin(), L.end()); }
        std::vector<float> tail (all.begin() + all.size() / 2, all.end());
        CHECK (toneDb (tail, 440) - toneDb (tail, 446) > 20, "A4 en DCO : énergie à 440 Hz %.1f dB vs 446 Hz %.1f dB", toneDb (tail, 440), toneDb (tail, 446));
    }

    // Repliement en sortie du moteur (voix suréchantillonnées ×2 + décimation)
    {
        SynthParams s; s.osc[0].wave = Waveform::Saw; s.osc[1].volume = 0.0f; s.cutoff = 18000; s.dco = true;
        s.ampEnv.sustain = 1.0f; SynthEngine e2; e2.prepare (SR); e2.setParams (s); e2.noteOn (111, 1.0f);
        const double f = midiToHz (111.0f);
        std::vector<float> all; for (int b = 0; b < 256; ++b) { e2.render (L.data(), R.data(), B); all.insert (all.end(), L.begin(), L.end()); }
        std::vector<float> tail (all.end() - (1 << 15), all.end());
        const double fund = toneDb (tail, f); double worst = -300; int worstH = 0;
        for (int h = 2; h <= 60; ++h)
        {
            if (h * f < SR / 2) continue;                            // vraie harmonique, pas un repli
            double fa = std::fmod (h * f, 2 * SR); if (fa > SR) fa = 2 * SR - fa;   // repli à 96 kHz
            if (fa > SR / 2) fa = SR - fa;                           // résidu après décimation
            if (fa < 15000) { double d = toneDb (tail, fa) - fund; if (d > worst) { worst = d; worstH = h; } }
        }
        CHECK (worst < -48, "Moteur, scie %.0f Hz : pire repli audible %.1f dB (harmonique %d)", f, worst, worstH);
    }
}

static void benchCpu()
{
    std::puts ("\n== Performance ==");
    SynthEngine eng; eng.prepare (SR); SynthParams p = fatPatch(); p.ampEnv.release = 2.0f; eng.setParams (p);
    for (int n = 0; n < 8; ++n) eng.noteOn (48 + n * 3, 0.9f);
    const int B = 256, blocks = (int) (SR * 5 / B); std::vector<float> L (B), R (B);
    auto t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b) eng.render (L.data(), R.data(), B);
    const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    std::printf ("       8 voix × 7 unissons × 2 osc : %.1f %% d'un cœur (5 s audio en %.2f s)\n", 100 * sec / 5.0, sec);
    p.unisonVoices = 1; eng.setParams (p);
    t0 = std::chrono::steady_clock::now();
    for (int b = 0; b < blocks; ++b) eng.render (L.data(), R.data(), B);
    const double sec1 = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    std::printf ("       8 voix sans unisson          : %.1f %% d'un cœur\n", 100 * sec1 / 5.0);
}

int main()
{
    testOscillators(); testFilters(); testEnvelope(); testEngine(); benchCpu();
    std::printf ("\n%s (%d échec(s))\n", failures ? "ÉCHECS" : "TOUS LES TESTS PASSENT", failures);
    return failures ? 1 : 0;
}
