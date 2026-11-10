#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

// Banque de samples 100% synthétisés (aucun fichier externe nécessaire).
namespace Eclipse
{
constexpr int numSamples = 24;

inline const char* const* sampleNames()
{
    static const char* n[numSamples] = {
        "Kick Rolling", "Kick Deep", "Kick Hard",
        "Clap Dark", "Snare Tight", "Rim Click",
        "Hat Closed", "Hat Open", "Hat Shimmer",
        "Tom Tribal", "Conga", "Metal Hit", "Zap Laser",
        "Bass Rolling", "Bass Acid", "Sub Drop",
        "Bleep Psy", "FM Bell", "Alien Voice", "Noise Riser",
        "Pad Pluck", "Glitch", "Sweep Down", "Arp Pluck" };
    return n;
}

struct SVF
{
    float low = 0, band = 0;
    // mode 0 = LP, 1 = BP, 2 = HP ; q = 1/Q
    float process (float in, float fc, float q, double sr, int mode)
    {
        fc = std::min (fc, (float) sr * 0.22f);
        const float f = 2.0f * std::sin (juce::MathConstants<float>::pi * fc / (float) sr);
        low += f * band;
        const float high = in - low - q * band;
        band += f * high;
        return mode == 0 ? low : (mode == 1 ? band : high);
    }
};

struct Osc
{
    double ph = 0;
    float sine (double f, double sr) { ph += f / sr; ph -= std::floor (ph); return (float) std::sin (juce::MathConstants<double>::twoPi * ph); }
    float saw  (double f, double sr) { ph += f / sr; ph -= std::floor (ph); return (float) (2.0 * ph - 1.0); }
    float sq   (double f, double sr) { ph += f / sr; ph -= std::floor (ph); return ph < 0.5 ? 1.0f : -1.0f; }
};

inline std::vector<float> generateSample (int idx, double sr)
{
    juce::Random rng (4242 + idx);
    auto noise = [&] { return rng.nextFloat() * 2.0f - 1.0f; };
    auto N = [&] (double s) { return (size_t) (s * sr); };
    auto ex = [] (double t, double tau) { return (float) std::exp (-t / tau); };
    const double TP = juce::MathConstants<double>::twoPi;
    std::vector<float> d;

    auto kick = [&] (double f0, double f1, double tauP, double tauA, double len, float drive, float click)
    {
        d.assign (N (len), 0.f); Osc o;
        for (size_t i = 0; i < d.size(); ++i)
        {
            double t = i / sr;
            float s = o.sine (f1 + (f0 - f1) * std::exp (-t / tauP), sr) * ex (t, tauA);
            s += noise() * ex (t, 0.002) * click;
            d[i] = std::tanh (s * drive);
        }
    };
    auto hat = [&] (double tau, double len, float ring)
    {
        static const double fr[6] = { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
        d.assign (N (len), 0.f); Osc o[6]; SVF hp;
        for (size_t i = 0; i < d.size(); ++i)
        {
            double t = i / sr; float s = 0;
            for (int k = 0; k < 6; ++k) s += o[k].sq (fr[k] * 7.0, sr);
            s = s * 0.15f * (1.0f + ring * std::sin ((float) (TP * 3100.0 * t))) + noise() * 0.4f;
            d[i] = hp.process (s, 7000.f, 1.0f, sr, 2) * ex (t, tau);
        }
    };

    switch (idx)
    {
    case 0: kick (180, 48, 0.03, 0.16, 0.4, 1.4f, 0.3f); break;
    case 1: kick (120, 38, 0.05, 0.28, 0.6, 1.2f, 0.1f); break;
    case 2: kick (250, 55, 0.02, 0.12, 0.35, 3.5f, 0.4f); break;
    case 3: { d.assign (N (0.3), 0.f); SVF bp;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr; float e = 0;
            for (double k : { 0.0, 0.011, 0.022 }) if (t >= k) e += ex (t - k, 0.004);
            if (t >= 0.033) e += 0.8f * ex (t - 0.033, 0.06);
            d[i] = bp.process (noise() * e, 1600.f, 0.5f, sr, 1); } break; }
    case 4: { d.assign (N (0.25), 0.f); Osc o; SVF hp;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = o.sine (180 + 60 * std::exp (-t / 0.02), sr) * ex (t, 0.07)
                 + hp.process (noise(), 1800.f, 1.f, sr, 2) * ex (t, 0.1) * 0.9f; } break; }
    case 5: { d.assign (N (0.06), 0.f);
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = (float) (std::sin (TP * 1700 * t) + std::sin (TP * 480 * t)) * ex (t, 0.006); } break; }
    case 6: hat (0.04, 0.15, 0.f); break;
    case 7: hat (0.25, 0.8, 0.f); break;
    case 8: hat (0.5, 1.2, 1.0f); break;
    case 9: { d.assign (N (0.45), 0.f); Osc o;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = o.sine (90 + 110 * std::exp (-t / 0.05), sr) * ex (t, 0.18); } break; }
    case 10: { d.assign (N (0.3), 0.f); Osc o;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = o.sine (250 + 120 * std::exp (-t / 0.012), sr) * ex (t, 0.1) + noise() * ex (t, 0.003) * 0.4f; } break; }
    case 11: { d.assign (N (0.9), 0.f); static const double r[6] = { 1, 1.47, 2.09, 2.56, 3.4, 4.1 }; Osc o[6];
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr; float s = 0;
            for (int k = 0; k < 6; ++k) s += o[k].sine (320 * r[k], sr) / (1.0f + k * 0.4f);
            d[i] = (s + noise() * ex (t, 0.003)) * ex (t, 0.18); } break; }
    case 12: { d.assign (N (0.3), 0.f); Osc o;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = o.sine (80 + 4000 * std::exp (-t / 0.05), sr) * ex (t, 0.12); } break; }
    case 13: { d.assign (N (0.25), 0.f); Osc o; SVF f;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            float s = f.process (o.saw (55, sr), 120.f + 900.f * ex (t, 0.03), 0.6f, sr, 0);
            d[i] = std::tanh (s * 2.f) * ex (t, 0.2) * (float) std::min (1.0, t / 0.002); } break; }
    case 14: { d.assign (N (0.35), 0.f); Osc o; SVF f;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            float s = f.process (o.saw (55, sr), 200.f + 2800.f * ex (t, 0.12), 0.18f, sr, 0);
            d[i] = std::tanh (s * 1.5f) * ex (t, 0.25); } break; }
    case 15: { d.assign (N (1.0), 0.f); Osc o;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = o.sine (30 + 70 * std::exp (-t / 0.25), sr) * ex (t, 0.6); } break; }
    case 16: { d.assign (N (0.2), 0.f); Osc o;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            double ph = TP * (o.ph = o.ph + (600 + 1400 * std::exp (-t / 0.04)) / sr);
            d[i] = (float) std::sin (ph + 2.5 * std::sin (ph * 2)) * ex (t, 0.08); } break; }
    case 17: { d.assign (N (1.5), 0.f); Osc c, m;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            float mod = m.sine (440 * 3.5, sr);
            c.ph += 440.0 / sr;
            d[i] = (float) std::sin (TP * c.ph + 4.0 * std::exp (-t / 0.4) * mod) * ex (t, 0.5); } break; }
    case 18: { d.assign (N (0.9), 0.f); Osc o; SVF a, b;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            float s = o.saw (220 * (1 + 0.03 * std::sin (TP * 6 * t)), sr);
            float f1 = 400.f + 500.f * (float) std::pow (std::sin (TP * 2.2 * t), 2.0);
            float f2 = 1200.f + 1200.f * (float) std::pow (std::sin (TP * 1.3 * t + 1.0), 2.0);
            d[i] = (a.process (s, f1, 0.1f, sr, 1) + 0.6f * b.process (s, f2, 0.1f, sr, 1))
                   * (float) std::sqrt (std::sin (juce::MathConstants<double>::pi * t / 0.9)); } break; }
    case 19: { d.assign (N (1.5), 0.f); SVF f;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            float a = (float) (t / 1.5);
            d[i] = f.process (noise(), 200.f * (float) std::pow (40.0, t / 1.5), 0.4f, sr, 0) * a * a; } break; }
    case 20: { d.assign (N (0.8), 0.f); Osc o[3]; SVF f; static const double dt[3] = { 0.994, 1.0, 1.006 };
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr; float s = 0;
            for (int k = 0; k < 3; ++k) s += o[k].saw (261.6 * dt[k], sr);
            d[i] = f.process (s, 200.f + 3000.f * ex (t, 0.25), 0.8f, sr, 0) * ex (t, 0.5); } break; }
    case 21: { d.assign (N (0.15), 0.f); float hold = 0; int cnt = 0;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            if (--cnt <= 0) { hold = std::round (noise() * 4.f) / 4.f; cnt = 40 + rng.nextInt (160); }
            d[i] = hold * ex (t, 0.06); } break; }
    case 22: { d.assign (N (1.0), 0.f); SVF f;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = f.process (noise(), 150.f + 6000.f * ex (t, 0.4), 0.15f, sr, 1) * ex (t, 0.5); } break; }
    default: { d.assign (N (0.3), 0.f); Osc o; SVF f;
        for (size_t i = 0; i < d.size(); ++i) { double t = i / sr;
            d[i] = f.process (o.sq (523.25, sr), 400.f + 4600.f * ex (t, 0.05), 0.5f, sr, 0) * ex (t, 0.12); } break; }
    }

    // normalisation + fondu de sortie anti-clic
    float pk = 1e-6f;
    for (float v : d) pk = std::max (pk, std::abs (v));
    for (auto& v : d) v *= 0.9f / pk;
    const size_t fade = std::min<size_t> (d.size(), 96);
    for (size_t i = 0; i < fade; ++i) d[d.size() - 1 - i] *= (float) i / (float) fade;
    return d;
}
} // namespace Eclipse
