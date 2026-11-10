#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

namespace Eclipse
{
// Delay stéréo psychédélique : modulation "tape wobble", ping-pong, diffusion (smear),
// filtre passe-bas/passe-haut dans la boucle et saturation douce.
class PsyDelay
{
public:
    struct Params { float timeSamples = 10000, feedback = 0.5f, toneHz = 5000, wobble = 0.3f,
                    wobbleRate = 0.4f, spread = 0.7f, smear = 0.3f, drive = 0.2f; };

    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (sr * 4.5);
        for (auto& b : buf) b.assign ((size_t) size, 0.f);
        pos = 0; phase = 0;
        for (int c = 0; c < 2; ++c)
        {
            lp[c] = hpState[c] = 0;
            ap[c][0].init ((int) (142 * sr / 44100.0)); ap[c][1].init ((int) (107 * sr / 44100.0) + c * 7);
        }
    }

    void process (float inL, float inR, float& outL, float& outR, const Params& p)
    {
        phase += p.wobbleRate / sampleRate; phase -= std::floor (phase);
        const float modSamples = p.wobble * 0.008f * (float) sampleRate;
        const float tp = juce::MathConstants<float>::twoPi;
        float yL = read (0, p.timeSamples + std::sin (tp * (float) phase) * modSamples);
        float yR = read (1, p.timeSamples + std::sin (tp * ((float) phase + 0.25f)) * modSamples);

        const float g = 0.55f * p.smear;
        yL = ap[0][1].process (ap[0][0].process (yL, g), g);
        yR = ap[1][1].process (ap[1][0].process (yR, g), g);

        float fbL = yL + (yR - yL) * p.spread;
        float fbR = yR + (yL - yR) * p.spread;

        const float a = 1.0f - std::exp (-tp * p.toneHz / (float) sampleRate);
        const float ah = 1.0f - std::exp (-tp * 120.0f / (float) sampleRate);
        const float gain = 1.0f + p.drive * 3.0f;
        float f[2] = { fbL, fbR };
        for (int c = 0; c < 2; ++c)
        {
            lp[c] += a * (f[c] - lp[c]);
            hpState[c] += ah * (lp[c] - hpState[c]);
            float x = lp[c] - hpState[c];
            f[c] = std::tanh (x * gain) / std::max (1.0f, gain * 0.6f);
        }
        buf[0][(size_t) pos] = inL + f[0] * p.feedback;
        buf[1][(size_t) pos] = inR + f[1] * p.feedback;
        if (++pos >= size) pos = 0;
        outL = yL; outR = yR;
    }

private:
    struct Allpass
    {
        std::vector<float> b; int i = 0;
        void init (int n) { b.assign ((size_t) std::max (n, 1), 0.f); i = 0; }
        float process (float in, float g)
        {
            float z = b[(size_t) i];
            float v = in - g * z;
            b[(size_t) i] = v;
            if (++i >= (int) b.size()) i = 0;
            return z + g * v;
        }
    };

    float read (int ch, float d) const
    {
        d = juce::jlimit (1.0f, (float) (size - 2), d);
        float rp = (float) pos - d; if (rp < 0) rp += (float) size;
        int i0 = (int) rp; int i1 = i0 + 1; if (i1 >= size) i1 = 0;
        float fr = rp - (float) i0;
        return buf[ch][(size_t) i0] + (buf[ch][(size_t) i1] - buf[ch][(size_t) i0]) * fr;
    }

    std::vector<float> buf[2];
    Allpass ap[2][2];
    double sampleRate = 44100, phase = 0;
    int size = 1, pos = 0;
    float lp[2] {}, hpState[2] {};
};
} // namespace Eclipse
