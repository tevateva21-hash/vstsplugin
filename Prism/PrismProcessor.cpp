#include "PrismProcessor.h"
#include "PrismEditor.h"
#include "PrismPresets.h"
using namespace Psy;
using APVTS = juce::AudioProcessorValueTreeState;

static const double syncBeats[8] = { 4.0, 2.0, 1.0, 0.5, 0.25, 1.0 / 3.0, 0.125, 1.0 / 6.0 };
static const double dlyBeats[8]  = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
static const float vowels[5][3] = {
    { 730.f, 1090.f, 2440.f }, { 530.f, 1840.f, 2480.f }, { 270.f, 2290.f, 3010.f },
    { 570.f, 840.f, 2410.f },  { 300.f, 870.f, 2240.f } };

APVTS::ParameterLayout PrismProcessor::createLayout()
{
    APVTS::ParameterLayout l; using R = juce::NormalisableRange<float>;
    auto pid = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };
    auto F = [&] (const char* id, const char* n, float lo, float hi, float def, float skew = 1.f)
    { l.add (std::make_unique<juce::AudioParameterFloat> (pid (id), n, R (lo, hi, 0.f, skew), def)); };
    auto C = [&] (const char* id, const char* n, juce::StringArray c, int def) { l.add (std::make_unique<juce::AudioParameterChoice> (pid (id), n, c, def)); };
    auto B = [&] (const char* id, const char* n, bool def) { l.add (std::make_unique<juce::AudioParameterBool> (pid (id), n, def)); };
    auto I = [&] (const char* id, const char* n, int lo, int hi, int def) { l.add (std::make_unique<juce::AudioParameterInt> (pid (id), n, lo, hi, def)); };

    F ("vol", "Volume", 0.f, 1.5f, 0.9f);
    // source / lecture
    C ("src", "Sample", { "Drone harmonique", "Reese sombre", "Choeur voyelles", "Texture metallique", "Accord mineur",
                          "Arpege acide", "Boucle percussive", "Pluck cristallin", "bruit cosmique", "Orgue rituel",
                          "Pad sub", "Grain glitch", "[ Mon sample ]" }, 0);
    C ("trig", "Declenchement", { "Boucle libre", "Clavier MIDI (retrig)", "Clavier MIDI (legato)" }, 0);
    C ("loopMode", "Boucle", { "Avant", "Aller-retour", "One-shot" }, 0);
    B ("rev", "Lecture inverse", false);
    F ("start", "Debut", 0.f, 0.95f, 0.f); F ("end", "Fin", 0.05f, 1.f, 1.f);
    F ("speed", "Vitesse", 0.1f, 4.f, 1.f, 0.5f);
    I ("root", "Note racine", 24, 96, 60); I ("semi", "Transpose", -24, 24, 0); F ("fine", "Fine", -50.f, 50.f, 0.f);
    F ("glide", "Glide", 0.f, 1.f, 0.05f, 0.5f);
    // granulaire
    B ("grnOn", "Granulaire on", false); F ("grnSize", "Taille grain (ms)", 15.f, 250.f, 80.f, 0.5f);
    F ("grnDens", "Densite", 2.f, 40.f, 14.f); F ("grnJit", "Jitter position", 0.f, 1.f, 0.25f);
    F ("grnSpread", "Stereo grains", 0.f, 1.f, 0.6f); F ("grnPitch", "Pitch jitter", 0.f, 1.f, 0.f);
    // filtres 1 et 2
    juce::StringArray ftypes { "LP 12", "LP 24", "HP 12", "HP 24", "BP 12", "BP 24", "Notch", "Ladder 24", "Comb +", "Comb -", "Formant" };
    B ("f1On", "Filtre 1 on", true); C ("f1Type", "F1 type", ftypes, 7);
    F ("f1Cut", "F1 cutoff", 20.f, 20000.f, 1200.f, 0.25f); F ("f1Res", "F1 reso", 0.f, 1.f, 0.45f);
    F ("f1Drive", "F1 drive", 0.f, 1.f, 0.25f); F ("f1Morph", "F1 voyelle", 0.f, 1.f, 0.f);
    F ("f1Key", "F1 suivi clavier", 0.f, 1.f, 0.f); F ("f1Pan", "F1 pan", -1.f, 1.f, 0.f);
    B ("f2On", "Filtre 2 on", false); C ("f2Type", "F2 type", ftypes, 4);
    F ("f2Cut", "F2 cutoff", 20.f, 20000.f, 2400.f, 0.25f); F ("f2Res", "F2 reso", 0.f, 1.f, 0.4f);
    F ("f2Drive", "F2 drive", 0.f, 1.f, 0.15f); F ("f2Morph", "F2 voyelle", 0.f, 1.f, 0.5f);
    F ("f2Key", "F2 suivi clavier", 0.f, 1.f, 0.f); F ("f2Pan", "F2 pan", -1.f, 1.f, 0.f);
    C ("routing", "Routage", { "Serie 1 -> 2", "Parallele" }, 0); F ("fMix", "Balance 1/2", 0.f, 1.f, 0.5f);
    // enveloppes
    F ("aAtk", "Amp A", 0.001f, 4.f, 0.01f, 0.35f); F ("aDec", "Amp D", 0.01f, 4.f, 0.3f, 0.35f);
    F ("aSus", "Amp S", 0.f, 1.f, 1.f); F ("aRel", "Amp R", 0.01f, 6.f, 0.4f, 0.35f);
    F ("fAtk", "Flt A", 0.001f, 4.f, 0.02f, 0.35f); F ("fDec", "Flt D", 0.01f, 4.f, 0.4f, 0.35f);
    F ("fSus", "Flt S", 0.f, 1.f, 0.2f); F ("fRel", "Flt R", 0.01f, 6.f, 0.5f, 0.35f);
    F ("fe1", "Env -> F1", -1.f, 1.f, 0.f); F ("fe2", "Env -> F2", -1.f, 1.f, 0.f); F ("velMod", "Velocite -> Filtre", 0.f, 1.f, 0.3f);
    // LFO 1 & 2
    juce::StringArray shapes { "Sinus", "Triangle", "Rampe +", "Rampe -", "Carre", "S&H", "Aleatoire lisse" };
    juce::StringArray syncs { "1/1", "1/2", "1/4", "1/8", "1/16", "1/8T", "1/32", "1/16T" };
    for (int i = 1; i <= 2; ++i)
    {
        auto p = "l" + juce::String (i);
        C ((p + "Shape").toRawUTF8(), "LFO forme", shapes, i == 1 ? 0 : 5);
        B ((p + "Sync").toRawUTF8(), "LFO sync", true);
        C ((p + "Div").toRawUTF8(), "LFO div", syncs, i == 1 ? 2 : 4);
        F ((p + "Rate").toRawUTF8(), "LFO Hz", 0.05f, 20.f, 1.f, 0.4f);
        F ((p + "C1").toRawUTF8(), "LFO -> F1", -1.f, 1.f, i == 1 ? 0.35f : 0.f);
        F ((p + "C2").toRawUTF8(), "LFO -> F2", -1.f, 1.f, 0.f);
        F ((p + "Res").toRawUTF8(), "LFO -> Res", -1.f, 1.f, 0.f);
        F ((p + "Pan").toRawUTF8(), "LFO -> Pan", -1.f, 1.f, 0.f);
        F ((p + "Pitch").toRawUTF8(), "LFO -> Pitch", -1.f, 1.f, 0.f);
    }
    // sequenceur de filtre
    B ("seqOn", "Seq on", true); I ("seqSteps", "Seq pas", 2, maxSteps, 16);
    C ("seqDiv", "Seq vitesse", { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, 3);
    F ("seqSmooth", "Lissage", 0.f, 1.f, 0.25f); F ("seqSwing", "Swing", 0.f, 1.f, 0.f);
    F ("seqC1", "Seq -> F1", -1.f, 1.f, 0.5f); F ("seqC2", "Seq -> F2", -1.f, 1.f, 0.f);
    F ("seqAmp", "Seq -> Gate", 0.f, 1.f, 0.f); F ("seqPitch", "Seq -> Pitch", -1.f, 1.f, 0.f);
    // effets finaux
    F ("crush", "Bitcrush", 0.f, 1.f, 0.f); F ("ring", "Ring mod", 0.f, 1.f, 0.f); F ("ringHz", "Ring Hz", 30.f, 1500.f, 180.f, 0.4f);
    F ("phRate", "Phaser vit", 0.05f, 5.f, 0.35f, 0.5f); F ("phDepth", "Phaser prof", 0.f, 1.f, 0.7f); F ("phMix", "Phaser mix", 0.f, 1.f, 0.f);
    C ("dlyDiv", "Delay div", { "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2" }, 3);
    F ("dlyFb", "Delay fb", 0.f, 0.95f, 0.5f); F ("dlyTone", "Delay tone", 300.f, 12000.f, 4000.f, 0.4f);
    F ("dlyWob", "Delay wobble", 0.f, 1.f, 0.3f); F ("dlySpread", "Ping-pong", 0.f, 1.f, 0.7f);
    F ("dlySmear", "Smear", 0.f, 1.f, 0.35f); F ("dlyMix", "Delay mix", 0.f, 1.f, 0.25f);
    F ("rvSize", "Reverb taille", 0.f, 1.f, 0.75f); F ("rvDamp", "Damping", 0.f, 1.f, 0.4f); F ("rvMix", "Reverb mix", 0.f, 1.f, 0.2f);
    return l;
}

PrismProcessor::PrismProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()), presets (prismPresets())
{
    formats.registerBasicFormats();
    setWave (2);
    makeBank();
    bankSr = sr;
}

void PrismProcessor::makeBank()
{
    bank.clear();
    const int N = (int) (sr * 4.0); // 4 s bouclables (220 * 4 = 880 cycles entiers, etc.)
    juce::Random r (123);
    for (int b = 0; b < 12; ++b)
    {
        std::vector<float> v ((size_t) N, 0.f);
        SVFT f1, f2;
        for (int i = 0; i < N; ++i)
        {
            const double t = (double) i / sr, u = (double) i / N, tp = kTwoPi;
            float s = 0;
            switch (b)
            {
                case 0: for (int k = 1; k <= 16; ++k) s += (float) std::sin (tp * 55.0 * k * t + k * 0.3 * std::sin (tp * 0.5 * u)) / (float) k; break;
                case 1: for (int k = -2; k <= 2; ++k) { double p = std::fmod ((55.0 + k * 0.25) * t, 1.0); s += (float) (2.0 * p - 1.0); } break;
                case 2: { for (int k = 1; k <= 20; ++k) s += (float) std::sin (tp * 110.0 * k * t) / std::sqrt ((float) k);
                          f1.process (s, tpG (600.f + 300.f * (float) std::sin (tp * u), sr), 0.12f);
                          f2.process (s, tpG (1500.f + 600.f * (float) std::cos (tp * u), sr), 0.12f); s = f1.bp + f2.bp; break; }
                case 3: { static const double rt[6] = { 1, 1.41, 1.73, 2.23, 2.82, 3.6 };
                          for (int k = 0; k < 6; ++k) s += (float) std::sin (tp * 110.0 * rt[k] * t + 2.0 * std::sin (tp * 220.0 * rt[k] * t)); break; }
                case 4: { static const double ch[5] = { 110, 130.81, 164.81, 196, 246.94 };
                          for (double f : ch) for (int h = 1; h <= 8; ++h) s += (float) std::sin (tp * f * h * t) / (float) h; break; }
                case 5: { static const double n[8] = { 55, 55, 110, 55, 82.4, 65.4, 110, 98 };
                          int st = (int) (u * 16.0) % 8; double loc = std::fmod (u * 16.0, 1.0), p = std::fmod (n[st] * t, 1.0);
                          s = (float) (2.0 * p - 1.0) * (float) std::exp (-loc * 3.5); break; }
                case 6: { double step = std::fmod (u * 16.0, 1.0); int si = (int) (u * 16.0);
                          float k = (si % 4 == 0) ? (float) std::sin (tp * (50 + 120 * std::exp (-step * 18)) * t) * (float) std::exp (-step * 6) : 0;
                          float h = (r.nextFloat() * 2.f - 1.f) * (float) std::exp (-step * (si % 2 ? 12 : 30)) * 0.6f; s = k + h; break; }
                case 7: for (int k = 1; k <= 12; ++k) s += (float) std::sin (tp * 220.0 * k * t) * (float) std::exp (-u * k * 1.5); break;
                case 8: { float nz = r.nextFloat() * 2.f - 1.f; f1.process (nz, tpG (300.f + 2500.f * (0.5f + 0.5f * (float) std::sin (tp * 2 * u)), sr), 0.2f);
                          s = f1.bp + 0.4f * (float) std::sin (tp * 55.0 * t); break; }
                case 9: { static const int hm[7] = { 1, 2, 3, 4, 6, 8, 12 };
                          for (int h : hm) s += (float) std::sin (tp * 110.0 * h * t) / std::sqrt ((float) h); break; }
                case 10: s = (float) (std::sin (tp * 55.0 * t) + 0.6 * std::sin (tp * 110.0 * t) + 0.4 * std::sin (tp * 165.0 * t) + 0.25 * std::sin (tp * 220.0 * t)); break;
                default: { int seg = (int) (u * 32.0); double f = 110.0 * std::pow (2.0, ((seg * 7) % 19 - 6) / 12.0);
                           s = (std::fmod (f * t, 1.0) < 0.5 ? 1.f : -1.f) * (float) std::exp (-std::fmod (u * 32.0, 1.0) * 4.0); break; }
            }
            v[(size_t) i] = s;
        }
        float pk = 1e-6f; for (float x : v) pk = std::max (pk, std::abs (x));
        for (float& x : v) x *= 0.85f / pk;
        const int fade = 256;
        for (int i = 0; i < fade; ++i)
        {
            const float a = (float) i / (float) fade;
            v[(size_t) (N - 1 - i)] = v[(size_t) (N - 1 - i)] * a + v[(size_t) i] * (1.f - a);
        }
        bank.push_back (std::move (v));
    }
}

void PrismProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    sr = sampleRate > 1000.0 ? sampleRate : 44100.0;
    const int safeBlock = std::max (64, blockSize);
    if (bank.empty() || std::abs (bankSr - sr) > 1.0)
    {
        makeBank();
        bankSr = sr;
    }
    if (userPath.isNotEmpty() && userData.load() == nullptr && juce::File (userPath).existsAsFile()) loadUserSample (juce::File (userPath), false);
    for (auto& c : flt) for (auto& f : c) f.init (sr);
    for (auto& g : grains) g.on = false;
    held.clear(); pos = 0; dir = 1; gate = false;
    delay.prepare (sr); reverb.reset();
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) safeBlock, 2 };
    phaser.prepare (spec); phaser.setCentreFrequency (900.f); phaser.setFeedback (0.55f);
    rvBuf.setSize (2, safeBlock);
}

const std::vector<float>* PrismProcessor::currentWave() const
{
    const int s = juce::jlimit (0, 12, (int) P ("src"));
    if (s == 12) if (auto* u = userData.load()) if (! u->empty()) return u;
    return bank.empty() ? nullptr : &bank[(size_t) std::min (s, (int) bank.size() - 1)];
}

bool PrismProcessor::loadUserSample (const juce::File& file, bool select)
{
    std::unique_ptr<juce::AudioFormatReader> rd (formats.createReaderFor (file));
    if (rd == nullptr || rd->lengthInSamples <= 0) return false;
    const double targetSr = sr > 1000.0 ? sr : 44100.0;
    const int len = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 30.0));
    juce::AudioBuffer<float> tmp ((int) std::max<unsigned> (1, rd->numChannels), len);
    rd->read (&tmp, 0, len, 0, true, true);
    const double ratio = rd->sampleRate / targetSr; const int outLen = std::max (64, (int) (len / ratio));
    auto out = std::make_unique<std::vector<float>> ((size_t) outLen, 0.f);
    float pk = 1e-6f;
    for (int i = 0; i < outLen; ++i)
    {
        const double p = i * ratio; const int i0 = std::min ((int) p, len - 1), i1 = std::min (i0 + 1, len - 1);
        const float fr = (float) (p - i0); float m = 0;
        for (int c = 0; c < tmp.getNumChannels(); ++c) { auto* d = tmp.getReadPointer (c); m += d[i0] + (d[i1] - d[i0]) * fr; }
        m /= (float) tmp.getNumChannels(); (*out)[(size_t) i] = m; pk = std::max (pk, std::abs (m));
    }
    const float g = pk > 0.9f ? 0.9f / pk : 1.f; for (auto& v : *out) v *= g;
    auto* raw = out.get(); graveyard.push_back (std::move (out)); userData = raw; userPath = file.getFullPathName();
    pos = 0;
    if (select) if (auto* p = apvts.getParameter ("src")) p->setValueNotifyingHost (p->convertTo0to1 (12.f));
    return true;
}

void PrismProcessor::setWave (int type)
{
    const int n = getSteps();
    for (int i = 0; i < maxSteps; ++i)
    {
        const float u = (float) (i % n) / (float) std::max (1, n - 1);
        float v = 0;
        switch (type)
        {
            case 0: v = 2.f * u - 1.f; break;
            case 1: v = 1.f - 2.f * u; break;
            case 2: v = 1.f - 4.f * std::abs (u - 0.5f); break;
            case 3: v = std::sin (kTwoPi * (float) i / (float) n); break;
            case 4: v = -1.f + 2.f * std::floor (u * 4.f) / 3.f; break;
            case 5: { static const float tr[8] = { 1.f, -0.6f, 0.4f, -0.8f, 0.8f, -0.6f, 0.3f, 0.9f }; v = tr[i % 8]; break; }
            case 6: v = rng.nextFloat() * 2.f - 1.f; break;
            default: v = 0.f; break;
        }
        seq[i] = v;
    }
}

float PrismProcessor::ADSR::tick (float a, float d, float s, float r, double sr)
{
    switch (st)
    {
        case 1: v += (1.1f - v) * (1.f - std::exp (-1.f / (a * (float) sr))); if (v >= 1.f) { v = 1.f; st = 2; } break;
        case 2: v += (s - v) * (1.f - std::exp (-1.f / (d * (float) sr))); break;
        case 4: v *= std::exp (-1.f / (r * (float) sr)); if (v < 1e-4f) { v = 0; st = 0; } break;
        default: break;
    }
    return v;
}

float PrismProcessor::MultiFilter::process (float x, int type, float fc, float res, float drive, float morph, double sr)
{
    fc = juce::jlimit (20.f, (float) sr * 0.44f, fc);
    const float drv = 1.f + drive * 6.f;
    x = std::tanh (x * drv) / std::sqrt (drv);
    const float g = tpG (fc, sr), k = 2.f - 1.93f * std::pow (res, 0.7f);
    switch (type)
    {
        case 0: s1.process (x, g, k); return s1.lp;
        case 1: s1.process (x, g, k); s2.process (s1.lp, g, k); return s2.lp;
        case 2: s1.process (x, g, k); return s1.hp;
        case 3: s1.process (x, g, k); s2.process (s1.hp, g, k); return s2.hp;
        case 4: s1.process (x, g, k); return s1.bp;
        case 5: s1.process (x, g, k); s2.process (s1.bp, g, k); return s2.bp;
        case 6: s1.process (x, g, k); return s1.lp + s1.hp;
        case 7: // ladder 4 poles
        {
            const float a = g / (1.f + g), fb = res * 3.95f;
            float u = std::tanh (x - fb * lad[3]);
            for (int i = 0; i < 4; ++i) { lad[i] += a * (u - lad[i]); u = lad[i]; }
            return lad[3];
        }
        case 8: case 9:
        {
            const float d = juce::jlimit (2.f, (float) comb.b.size() - 4.f, (float) sr / fc);
            const float y = comb.read (d), fb = (type == 8 ? 1.f : -1.f) * (0.2f + 0.76f * res);
            comb.write (std::tanh (x + y * fb));
            return (x + y) * 0.7f;
        }
        default: // formant
        {
            const float m = morph * 4.f; const int v0 = std::min (3, (int) m); const float fr = m - (float) v0;
            const float scale = fc / 1000.f, q = 0.08f + (1.f - res) * 0.18f;
            float sum = 0;
            for (int b = 0; b < 3; ++b)
            {
                const float vf = (vowels[v0][b] + (vowels[v0 + 1][b] - vowels[v0][b]) * fr) * scale;
                f[b].process (x, tpG (vf, sr), q);
                sum += f[b].bp / (1.f + 0.4f * (float) b);
            }
            return sum * 0.55f;
        }
    }
}

void PrismProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    const auto* w = currentWave();
    if (w == nullptr || w->empty()) return;
    const int W = (int) w->size();
    const int numOutCh = buffer.getNumChannels();
    if (numOutCh < 1) return;
    auto* oL = buffer.getWritePointer (0);
    auto* oR = numOutCh > 1 ? buffer.getWritePointer (1) : oL;
    if (rvBuf.getNumSamples() < n) rvBuf.setSize (2, n, false, false, true);

    // tempo / position
    double bpm = 135.0, ppq = internalPpq; bool hostPpq = false;
    if (auto* ph = getPlayHead()) if (auto p = ph->getPosition())
    {
        if (auto b = p->getBpm()) bpm = *b;
        if (p->getIsPlaying()) if (auto q = p->getPpqPosition()) { ppq = *q; hostPpq = true; }
    }
    const double pps = bpm / 60.0 / sr;

    // parametres
    const int trig = (int) P ("trig"), loopMode = (int) P ("loopMode");
    const bool rev = P ("rev") > 0.5f, grnOn = P ("grnOn") > 0.5f, seqOn = P ("seqOn") > 0.5f;
    const float s0 = std::min (P ("start"), P ("end") - 0.02f), s1 = std::max (P ("end"), s0 + 0.02f);
    const double startIdx = s0 * (W - 1), endIdx = s1 * (W - 1), span = std::max (32.0, endIdx - startIdx);
    const float speed = P ("speed"), root = P ("root"), semi = P ("semi"), fine = P ("fine"), glide = P ("glide");
    const float glideC = glide < 0.002f ? 1.f : 1.f - std::exp (-1.f / (glide * (float) sr));
    const float aAtk = P ("aAtk"), aDec = P ("aDec"), aSus = P ("aSus"), aRel = P ("aRel");
    const float fAtk = P ("fAtk"), fDec = P ("fDec"), fSus = P ("fSus"), fRel = P ("fRel");
    const float fe1 = P ("fe1"), fe2 = P ("fe2"), velMod = P ("velMod");
    const bool f1On = P ("f1On") > 0.5f, f2On = P ("f2On") > 0.5f;
    const int f1Type = (int) P ("f1Type"), f2Type = (int) P ("f2Type"), routing = (int) P ("routing");
    const float f1Cut = P ("f1Cut"), f1Res = P ("f1Res"), f1Drv = P ("f1Drive"), f1Mrp = P ("f1Morph"), f1Key = P ("f1Key"), f1Pan = P ("f1Pan");
    const float f2Cut = P ("f2Cut"), f2Res = P ("f2Res"), f2Drv = P ("f2Drive"), f2Mrp = P ("f2Morph"), f2Key = P ("f2Key"), f2Pan = P ("f2Pan");
    const float fMix = P ("fMix");
    const int l1Sh = (int) P ("l1Shape"), l2Sh = (int) P ("l2Shape");
    const double l1Hz = P ("l1Sync") > 0.5f ? (bpm / 60.0) / syncBeats[(int) P ("l1Div")] : (double) P ("l1Rate");
    const double l2Hz = P ("l2Sync") > 0.5f ? (bpm / 60.0) / syncBeats[(int) P ("l2Div")] : (double) P ("l2Rate");
    const float l1C1 = P ("l1C1"), l1C2 = P ("l1C2"), l1Res = P ("l1Res"), l1Pan = P ("l1Pan"), l1Pit = P ("l1Pitch");
    const float l2C1 = P ("l2C1"), l2C2 = P ("l2C2"), l2Res = P ("l2Res"), l2Pan = P ("l2Pan"), l2Pit = P ("l2Pitch");
    static const double seqBeats[6] = { 1.0, 0.5, 1.0 / 3.0, 0.25, 1.0 / 6.0, 0.125 };
    const double seqStepLen = seqBeats[(int) P ("seqDiv")]; const int seqSteps = getSteps();
    const float seqSmoothC = 1.f - std::exp (-1.f / (std::max (0.0005f, P ("seqSmooth") * 0.08f) * (float) sr));
    const float seqSwing = P ("seqSwing"), seqC1 = P ("seqC1"), seqC2 = P ("seqC2"), seqAmp = P ("seqAmp"), seqPitch = P ("seqPitch");
    const float crush = P ("crush"), ring = P ("ring"), ringHz = P ("ringHz"), vol = P ("vol");

    auto readSmp = [&] (double p) -> float
    {
        p = juce::jlimit (0.0, (double) (W - 1), p);
        const int i0 = (int) p, i1 = std::min (i0 + 1, W - 1); const float fr = (float) (p - i0);
        return (*w)[(size_t) i0] + ((*w)[(size_t) i1] - (*w)[(size_t) i0]) * fr;
    };

    auto midiIt = midi.begin();
    double ringPh = 0;
    for (int i = 0; i < n; ++i)
    {
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i)
        {
            auto m = (*midiIt).getMessage(); ++midiIt;
            if (m.isNoteOn())
            {
                const int k = m.getNoteNumber(); const bool wasEmpty = held.empty();
                held.erase (std::remove (held.begin(), held.end(), k), held.end()); held.push_back (k);
                targetNote = (float) k; vel = m.getFloatVelocity();
                if (wasEmpty) curNote = targetNote;
                const bool retrig = trig != 2 || wasEmpty;
                ampEnv.noteOn (retrig); fltEnv.noteOn (retrig);
                if (retrig) { pos = rev ? endIdx : startIdx; dir = rev ? -1 : 1; }
            }
            else if (m.isNoteOff())
            {
                held.erase (std::remove (held.begin(), held.end(), m.getNoteNumber()), held.end());
                if (! held.empty()) targetNote = (float) held.back();
                else if (trig != 0) { ampEnv.noteOff(); fltEnv.noteOff(); }
            }
        }
        curNote += (targetNote - curNote) * glideC;
        if (trig == 0 && ampEnv.st == 0)
        {
            ampEnv.noteOn (true); fltEnv.noteOn (true);
            if (held.empty()) curNote = targetNote = root;
            pos = juce::jlimit (startIdx, endIdx, pos);
        }

        // LFOs & sequenceur
        const float v1 = lfo1.tick (l1Hz / sr, l1Sh), v2 = lfo2.tick (l2Hz / sr, l2Sh);
        float sv = 0, stepSub = 0;
        if (seqOn)
        {
            const double pair = (ppq + i * pps) / seqStepLen / 2.0, pi = std::floor (pair), f = pair - pi, b = 0.5 + 0.25 * seqSwing;
            const int k = f < b ? 0 : 1; stepSub = (float) (k == 0 ? f / b : (f - b) / (1.0 - b));
            const int st = (int) ((((long long) pi * 2 + k) % seqSteps + seqSteps) % seqSteps);
            curStep = st; sv = seq[st];
        }
        else curStep = -1;
        seqSmoothed += (sv - seqSmoothed) * seqSmoothC;

        const float aEnv = trig == 0 ? 1.f : ampEnv.tick (aAtk, aDec, aSus, aRel, sr);
        const float fEnv = fltEnv.tick (fAtk, fDec, fSus, fRel, sr);

        // lecture du sample
        const float pitchSemis = (curNote - root) + semi + fine * 0.01f + (v1 * l1Pit + v2 * l2Pit + seqSmoothed * seqPitch) * 12.f;
        const double rate = speed * std::pow (2.0, pitchSemis / 12.0);
        float sL = 0, sR = 0;
        if (grnOn)
        {
            if (--grainCooldown <= 0)
            {
                grainCooldown = std::max (8, (int) (sr / std::max (1.f, P ("grnDens"))));
                for (auto& g : grains)
                    if (! g.on)
                    {
                        g.on = true; g.age = 0; g.len = std::max (64, (int) (P ("grnSize") * 0.001f * (float) sr));
                        const double jit = (rng.nextDouble() - 0.5) * P ("grnJit") * span;
                        g.pos = juce::jlimit (startIdx, endIdx, pos + jit);
                        const double pjit = std::pow (2.0, (rng.nextFloat() - 0.5f) * P ("grnPitch"));
                        g.inc = (rev ? -rate : rate) * pjit;
                        const float pan = (rng.nextFloat() - 0.5f) * 2.f * P ("grnSpread"), ang = (pan + 1.f) * kPi * 0.25f;
                        g.panL = std::cos (ang); g.panR = std::sin (ang); break;
                    }
            }
            for (auto& g : grains)
            {
                if (! g.on) continue;
                const float wEnv = std::sin (kPi * (float) g.age / (float) g.len);
                const float y = readSmp (g.pos) * wEnv;
                sL += y * g.panL; sR += y * g.panR;
                g.pos += g.inc;
                if (g.pos > endIdx) g.pos -= span; else if (g.pos < startIdx) g.pos += span;
                if (++g.age >= g.len) g.on = false;
            }
            sL *= 0.45f; sR *= 0.45f;
            pos += (rev ? -rate : rate) * 0.35;
            if (pos > endIdx) pos -= span; else if (pos < startIdx) pos += span;
        }
        else
        {
            const float y = readSmp (pos);
            // fondu aux bornes de boucle pour eviter les clics
            const double dEdge = std::min (pos - startIdx, endIdx - pos);
            const float edge = dEdge < 128.0 ? (float) (dEdge / 128.0) : 1.f;
            sL = sR = y * edge;
            pos += rate * (rev ? -dir : dir);
            if (loopMode == 0) { if (pos > endIdx) pos = startIdx + std::fmod (pos - startIdx, span); else if (pos < startIdx) pos = endIdx - std::fmod (endIdx - pos, span); }
            else if (loopMode == 1) { if (pos >= endIdx) { pos = endIdx; dir = -1; } else if (pos <= startIdx) { pos = startIdx; dir = 1; } }
            else { if (pos > endIdx || pos < startIdx) { sL = sR = 0; } }
        }
        playRatio = (float) ((pos - startIdx) / span);

        // calcul des cutoffs (en octaves)
        const float keyOct = (curNote - 60.f) / 12.f, velOct = (vel - 0.5f) * velMod * 2.f;
        const float oct1 = fEnv * fe1 * 5.f + (v1 * l1C1 + v2 * l2C1) * 4.f + seqSmoothed * seqC1 * 4.f + keyOct * f1Key + velOct;
        const float oct2 = fEnv * fe2 * 5.f + (v1 * l1C2 + v2 * l2C2) * 4.f + seqSmoothed * seqC2 * 4.f + keyOct * f2Key + velOct;
        const float c1 = juce::jlimit (20.f, 20000.f, f1Cut * std::pow (2.f, oct1));
        const float c2 = juce::jlimit (20.f, 20000.f, f2Cut * std::pow (2.f, oct2));
        const float r1 = juce::jlimit (0.f, 0.98f, f1Res + (v1 * l1Res + v2 * l2Res) * 0.4f);
        const float r2 = juce::jlimit (0.f, 0.98f, f2Res + (v1 * l1Res + v2 * l2Res) * 0.4f);
        if ((i & 31) == 0) { modCut1 = c1; modCut2 = c2; }

        float l = sL, r = sR;
        if (routing == 0)
        {
            if (f1On) { l = flt[0][0].process (l, f1Type, c1, r1, f1Drv, f1Mrp, sr); r = flt[0][1].process (r, f1Type, c1, r1, f1Drv, f1Mrp, sr); }
            if (f2On) { l = flt[1][0].process (l, f2Type, c2, r2, f2Drv, f2Mrp, sr); r = flt[1][1].process (r, f2Type, c2, r2, f2Drv, f2Mrp, sr); }
        }
        else
        {
            const float a1L = std::cos ((f1Pan + 1.f) * kPi * 0.25f), a1R = std::sin ((f1Pan + 1.f) * kPi * 0.25f);
            const float a2L = std::cos ((f2Pan + 1.f) * kPi * 0.25f), a2R = std::sin ((f2Pan + 1.f) * kPi * 0.25f);
            const float x1L = f1On ? flt[0][0].process (l, f1Type, c1, r1, f1Drv, f1Mrp, sr) * a1L : l;
            const float x1R = f1On ? flt[0][1].process (r, f1Type, c1, r1, f1Drv, f1Mrp, sr) * a1R : r;
            const float x2L = f2On ? flt[1][0].process (l, f2Type, c2, r2, f2Drv, f2Mrp, sr) * a2L : 0.f;
            const float x2R = f2On ? flt[1][1].process (r, f2Type, c2, r2, f2Drv, f2Mrp, sr) * a2R : 0.f;
            l = x1L * (1.f - fMix) + x2L * fMix; r = x1R * (1.f - fMix) + x2R * fMix;
        }

        // gate de sequenceur + pan LFO + enveloppe d'ampli
        float gEnv = aEnv;
        if (seqOn && seqAmp > 0.01f)
        {
            const float trGate = (sv > 0.f && stepSub < 0.65f) ? (0.3f + 0.7f * sv) : 0.05f;
            gEnv *= (1.f - seqAmp) + seqAmp * trGate;
        }
        const float panMod = juce::jlimit (-1.f, 1.f, v1 * l1Pan + v2 * l2Pan);
        const float pAng = (panMod + 1.f) * kPi * 0.25f;
        l *= gEnv * std::cos (pAng) * 1.4f; r *= gEnv * std::sin (pAng) * 1.4f;

        if (crush > 0.01f)
        {
            const int hold = 1 + (int) (crush * 18.f); const float q = std::max (2.f, 32.f * (1.f - crush * 0.9f));
            if (--holdCnt <= 0) { heldL = std::round (l * q) / q; heldR = std::round (r * q) / q; holdCnt = hold; }
            l = heldL; r = heldR;
        }
        if (ring > 0.01f)
        {
            ringPh += ringHz / sr; ringPh -= std::floor (ringPh);
            const float m = (float) std::sin (kTwoPi * ringPh);
            l = l * (1.f - ring) + l * m * ring; r = r * (1.f - ring) + r * m * ring;
        }
        oL[i] = l; oR[i] = r;
    }
    if (! hostPpq) internalPpq += n * pps;

    // phaser, delay, reverb
    if (P ("phMix") > 0.001f)
    {
        phaser.setRate (P ("phRate")); phaser.setDepth (P ("phDepth")); phaser.setMix (P ("phMix"));
        juce::dsp::AudioBlock<float> blk (buffer); phaser.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }
    const float dlyMix = P ("dlyMix");
    if (dlyMix > 0.001f)
    {
        const double target = dlyBeats[(int) P ("dlyDiv")] * 60.0 / bpm * sr;
        Eclipse::PsyDelay::Params dp { (float) delayTime, P ("dlyFb"), P ("dlyTone"), P ("dlyWob"), 0.4f, P ("dlySpread"), P ("dlySmear"), 0.25f };
        for (int i = 0; i < n; ++i)
        {
            delayTime += (target - delayTime) * 0.0004; dp.timeSamples = (float) delayTime;
            float yl, yr; delay.process (oL[i], oR[i], yl, yr, dp); oL[i] += yl * dlyMix; oR[i] += yr * dlyMix;
        }
    }
    const float rvMix = P ("rvMix");
    if (rvMix > 0.001f)
    {
        rvBuf.copyFrom (0, 0, buffer, 0, 0, n); rvBuf.copyFrom (1, 0, buffer, 1, 0, n);
        reverb.setParameters ({ P ("rvSize"), P ("rvDamp"), 1.f, 0.f, 1.f, 0.f });
        reverb.processStereo (rvBuf.getWritePointer (0), rvBuf.getWritePointer (1), n);
        for (int i = 0; i < n; ++i) { oL[i] += rvBuf.getSample (0, i) * rvMix; oR[i] += rvBuf.getSample (1, i) * rvMix; }
    }
    for (int i = 0; i < n; ++i) { oL[i] = std::tanh (oL[i] * vol); oR[i] = std::tanh (oR[i] * vol); }
}

void PrismProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto xml = apvts.copyState().createXml();
    juce::String s; for (int i = 0; i < maxSteps; ++i) s << seq[i].load() << ",";
    xml->setAttribute ("seq", s); xml->setAttribute ("userPath", userPath);
    copyXmlToBinary (*xml, dest);
}

void PrismProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto parts = juce::StringArray::fromTokens (xml->getStringAttribute ("seq"), ",", "");
        for (int i = 0; i < maxSteps && i < parts.size(); ++i) seq[i] = parts[i].getFloatValue();
        auto p = xml->getStringAttribute ("userPath");
        if (p.isNotEmpty() && juce::File (p).existsAsFile()) loadUserSample (juce::File (p), false);
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessorEditor* PrismProcessor::createEditor() { return new PrismEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PrismProcessor(); }
