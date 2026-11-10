#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>
#include <algorithm>

// Briques DSP communes aux plugins PsyLab.
namespace Psy
{
constexpr float kPi = 3.14159265358979f, kTwoPi = 6.28318530717959f;

struct OnePole
{
    float z = 0;
    float lp (float x, float a) { z += a * (x - z); return z; }
    static float coef (float fc, double sr) { return 1.f - std::exp (-kTwoPi * std::min (fc, (float) sr * 0.45f) / (float) sr); }
};

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float process (float x) { float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
    void set (float nb0, float nb1, float nb2, float a0, float na1, float na2)
    { b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0; }
    static float cl (float f, double sr) { return std::max (10.f, std::min (f, (float) sr * 0.45f)); }
    void lowpass (double sr, float f, float q)  { float w = kTwoPi * cl (f, sr) / (float) sr, c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
    void highpass (double sr, float f, float q) { float w = kTwoPi * cl (f, sr) / (float) sr, c = std::cos (w), al = std::sin (w) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
    void peak (double sr, float f, float q, float gdb)
    {
        float A = std::pow (10.f, gdb / 40.f), w = kTwoPi * cl (f, sr) / (float) sr, c = std::cos (w), al = std::sin (w) / (2 * q);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double sr, float f, float gdb)
    {
        float A = std::pow (10.f, gdb / 40.f), w = kTwoPi * cl (f, sr) / (float) sr, c = std::cos (w), s = std::sin (w);
        float al = s / 2 * std::sqrt (2.f), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
             (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void highShelf (double sr, float f, float gdb)
    {
        float A = std::pow (10.f, gdb / 40.f), w = kTwoPi * cl (f, sr) / (float) sr, c = std::cos (w), s = std::sin (w);
        float al = s / 2 * std::sqrt (2.f), sq = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
             (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
};

// SVF "topology preserving" 12 dB. k = 1/Q.
struct SVFT
{
    float s1 = 0, s2 = 0, lp = 0, bp = 0, hp = 0;
    void process (float x, float g, float k)
    {
        hp = (x - (g + k) * s1 - s2) / (1.f + g * (g + k));
        bp = g * hp + s1; lp = g * bp + s2;
        s1 = g * hp + bp; s2 = g * bp + lp;
    }
    void reset() { s1 = s2 = lp = bp = hp = 0; }
};

inline float tpG (float fc, double sr) { return std::tan (kPi * std::max (10.f, std::min (fc, (float) sr * 0.45f)) / (float) sr); }

struct DelayLine
{
    std::vector<float> b; int w = 0;
    void init (int n) { b.assign ((size_t) std::max (n, 8) + 4, 0.f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.f); }
    void write (float x) { b[(size_t) w] = x; if (++w >= (int) b.size()) w = 0; }
    float read (float d) const
    {
        const int n = (int) b.size(); d = std::max (1.f, std::min (d, (float) (n - 2)));
        float rp = (float) w - d; if (rp < 0) rp += (float) n;
        int i0 = (int) rp, i1 = i0 + 1; if (i1 >= n) i1 = 0;
        float f = rp - (float) i0; return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * f;
    }
};

struct Allpass
{
    std::vector<float> b; int i = 0;
    void init (int n) { b.assign ((size_t) std::max (n, 1), 0.f); i = 0; }
    float process (float in, float g)
    {
        float z = b[(size_t) i], v = in - g * z; b[(size_t) i] = v;
        if (++i >= (int) b.size()) i = 0;
        return z + g * v;
    }
};

// shape : 0 sinus, 1 triangle, 2 dent montante, 3 dent descendante, 4 carre, 5 sample&hold, 6 aleatoire lisse
struct LFO
{
    double ph = 0; float held = 0, target = 0, cur = 0; juce::Random rnd { 7 };
    void retrigger() { ph = 0; }
    float tick (double inc, int shape)
    {
        ph += inc; bool wrap = false; if (ph >= 1.0) { ph -= std::floor (ph); wrap = true; }
        const float p = (float) ph;
        switch (shape)
        {
            case 0: return std::sin (kTwoPi * p);
            case 1: return 4.f * std::abs (p - 0.5f) - 1.f;
            case 2: return 2.f * p - 1.f;
            case 3: return 1.f - 2.f * p;
            case 4: return p < 0.5f ? 1.f : -1.f;
            case 5: if (wrap) held = rnd.nextFloat() * 2.f - 1.f; return held;
            default: if (wrap) target = rnd.nextFloat() * 2.f - 1.f;
                     cur += (target - cur) * (float) std::min (1.0, inc * 8.0); return cur;
        }
    }
};

struct PitchShifter
{
    std::vector<float> b; int w = 0; float phase = 0; static constexpr int W = 2048;
    void init() { b.assign (8192, 0.f); w = 0; phase = 0; }
    float tap (float d) const
    {
        float rp = (float) w - d; if (rp < 0) rp += 8192.f;
        int i0 = (int) rp & 8191, i1 = (i0 + 1) & 8191; float f = rp - std::floor (rp);
        return b[(size_t) i0] + (b[(size_t) i1] - b[(size_t) i0]) * f;
    }
    float process (float x, float ratio)
    {
        b[(size_t) w] = x; w = (w + 1) & 8191;
        phase += (1.f - ratio) / (float) W; phase -= std::floor (phase);
        float p2 = phase + 0.5f; p2 -= std::floor (p2);
        float s1 = std::sin (kPi * phase), s2 = std::sin (kPi * p2);
        return tap (phase * W + 2.f) * s1 * s1 + tap (p2 * W + 2.f) * s2 * s2;
    }
};

// chorus / flanger / vibrato
struct ModDelay
{
    DelayLine dl[2]; double ph = 0, sr = 44100;
    void init (double s) { sr = s; dl[0].init ((int) (s * 0.06)); dl[1].init ((int) (s * 0.06)); }
    void process (float& l, float& r, float rate, float depthMs, float centreMs, float fb, float mix)
    {
        ph += rate / sr; ph -= std::floor (ph);
        float in[2] = { l, r }, out[2];
        for (int c = 0; c < 2; ++c)
        {
            float lfo = std::sin (kTwoPi * ((float) ph + 0.25f * (float) c));
            float y = dl[c].read ((centreMs + depthMs * lfo) * (float) sr / 1000.f);
            dl[c].write (in[c] + fb * y);
            out[c] = in[c] + (y - in[c]) * mix;
        }
        l = out[0]; r = out[1];
    }
};

// haut-parleur rotatif (cornet + tambour)
struct Leslie
{
    DelayLine hornD[2], drumD[2]; OnePole lo; double sr = 44100, hornPh = 0, drumPh = 0; float hornSpeed = 0.8f, drumSpeed = 0.7f;
    void init (double s) { sr = s; for (int c = 0; c < 2; ++c) { hornD[c].init ((int) (s * 0.01)); drumD[c].init ((int) (s * 0.01)); } }
    void process (float& l, float& r, bool fast, float depth, float balance, float mix)
    {
        const float in = (l + r) * 0.5f, low = lo.lp (in, OnePole::coef (800.f, sr)), hi = in - low;
        hornSpeed += ((fast ? 6.6f : 0.8f) - hornSpeed) * (1.f - std::exp (-1.f / (0.6f * (float) sr)));
        drumSpeed += ((fast ? 5.9f : 0.7f) - drumSpeed) * (1.f - std::exp (-1.f / (2.5f * (float) sr)));
        hornPh += hornSpeed / sr; hornPh -= std::floor (hornPh);
        drumPh += drumSpeed / sr; drumPh -= std::floor (drumPh);
        float o[2];
        for (int c = 0; c < 2; ++c)
        {
            const float hp = kTwoPi * ((float) hornPh + 0.25f * (float) c), dp = kTwoPi * ((float) drumPh + 0.25f * (float) c);
            float yh = hornD[c].read ((0.0006f + 0.0005f * depth * std::sin (hp)) * (float) sr) * (1.f + 0.5f * depth * std::cos (hp));
            float yd = drumD[c].read ((0.0010f + 0.0003f * depth * std::sin (dp)) * (float) sr) * (1.f + 0.35f * depth * std::cos (dp));
            hornD[c].write (hi); drumD[c].write (low);
            o[c] = (yh * balance + yd * (1.f - balance)) * 1.2f;
        }
        l += (o[0] - l) * mix; r += (o[1] - r) * mix;
    }
};

struct SpringReverb
{
    DelayLine d; Allpass ap[4]; OnePole lpf; double sr = 44100;
    void init (double s)
    {
        sr = s; d.init ((int) (s * 0.06));
        const int len[4] = { 142, 210, 330, 470 };
        for (int i = 0; i < 4; ++i) ap[i].init ((int) (len[i] * s / 44100.0));
    }
    void process (float in, float decay, float damp, float& oL, float& oR)
    {
        float y = d.read (0.033f * (float) sr), z = y;
        for (auto& a : ap) z = a.process (z, 0.65f);
        z = lpf.lp (z, OnePole::coef (1500.f + 5000.f * damp, sr));
        d.write (in + z * decay);
        oL = y; oR = d.read (0.0237f * (float) sr);
    }
};
} // namespace Psy
