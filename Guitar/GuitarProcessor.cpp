#include "GuitarProcessor.h"
#include "GuitarEditor.h"
#include "GuitarPresets.h"
using namespace Psy;
using APVTS = juce::AudioProcessorValueTreeState;

static const double dlyBeats[8] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };
static const std::vector<std::vector<int>>& chords()
{
    static const std::vector<std::vector<int>> c = { {0}, {0,7,12}, {0,12}, {0,4,7,12}, {0,3,7,12}, {0,2,7,12}, {0,5,7,12},
                                                     {0,4,7,14}, {0,4,7,11}, {0,3,7,10}, {0,3,7,10,14}, {0,7,12,19,24} };
    return c;
}

APVTS::ParameterLayout GuitarProcessor::createLayout()
{
    APVTS::ParameterLayout l;
    using R = juce::NormalisableRange<float>;
    auto pid = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };
    auto F = [&] (const char* id, const char* n, float lo, float hi, float def, float skew = 1.f)
    { l.add (std::make_unique<juce::AudioParameterFloat> (pid (id), n, R (lo, hi, 0.f, skew), def)); };
    auto C = [&] (const char* id, const char* n, juce::StringArray c, int def)
    { l.add (std::make_unique<juce::AudioParameterChoice> (pid (id), n, c, def)); };
    auto B = [&] (const char* id, const char* n, bool def) { l.add (std::make_unique<juce::AudioParameterBool> (pid (id), n, def)); };
    auto I = [&] (const char* id, const char* n, int lo, int hi, int def) { l.add (std::make_unique<juce::AudioParameterInt> (pid (id), n, lo, hi, def)); };

    // instrument
    F ("vol", "Volume", 0.f, 1.5f, 0.9f);
    F ("pickPos", "Position mediator", 0.05f, 0.5f, 0.2f); F ("pickHard", "Durete mediator", 0.f, 1.f, 0.6f);
    F ("pickNoise", "Bruit de mediator", 0.f, 1.f, 0.2f); F ("body", "Corps", 0.f, 1.f, 0.4f);
    F ("bright", "Brillance", 0.f, 1.f, 0.6f); F ("sustain", "Sustain (s)", 0.2f, 15.f, 3.f, 0.4f);
    F ("sustainPlus", "Sustain infini", 0.f, 1.f, 0.f);
    C ("mode", "Mode", { "Poly", "Mono Legato" }, 0);
    C ("chord", "Accord", { "Off", "Power 5", "Octave", "Majeur", "Mineur", "Sus2", "Sus4", "Add9", "Maj7", "Min7", "Min9", "Open spread" }, 0);
    C ("strumDir", "Sens du strum", { "Bas", "Haut", "Alterne" }, 0);
    F ("strumMs", "Strum (ms)", 0.f, 120.f, 25.f); F ("glide", "Glide (s)", 0.f, 1.f, 0.05f, 0.5f);
    F ("bendRange", "Bend (demi-tons)", 0.f, 12.f, 2.f); F ("vibRate", "Vibrato vitesse", 0.5f, 10.f, 5.f);
    F ("vibDepth", "Vibrato profondeur", 0.f, 1.f, 0.1f);
    I ("transpose", "Transpose", -24, 24, 0); F ("fine", "Fine (cents)", -50.f, 50.f, 0.f);
    F ("twelve", "12 cordes", 0.f, 1.f, 0.f); F ("spread", "Largeur stereo", 0.f, 1.f, 0.4f);
    // compresseur
    B ("compOn", "Comp on", false); F ("compThr", "Comp seuil", -50.f, 0.f, -20.f); F ("compRatio", "Comp ratio", 1.f, 20.f, 4.f, 0.5f);
    F ("compAtk", "Comp attaque", 1.f, 100.f, 10.f, 0.5f); F ("compRel", "Comp release", 20.f, 1000.f, 250.f, 0.5f); F ("compMakeup", "Comp gain", 0.f, 24.f, 6.f);
    // octaver
    B ("octOn", "Octaver on", false); F ("octDown", "Octave -1", 0.f, 1.f, 0.5f); F ("octUp", "Octave +1", 0.f, 1.f, 0.f);
    // drive
    B ("drvOn", "Drive on", false); C ("drvType", "Type drive", { "Boost", "Overdrive", "Fuzz", "Big Muff", "Distortion" }, 3);
    F ("drvGain", "Drive gain", 0.f, 1.f, 0.6f); F ("drvTone", "Drive tone", 0.f, 1.f, 0.5f);
    F ("drvLevel", "Drive niveau", 0.f, 1.5f, 0.7f); F ("drvMix", "Drive mix", 0.f, 1.f, 1.f);
    // wah
    B ("wahOn", "Wah on", false); C ("wahMode", "Wah mode", { "Pedale", "Auto", "LFO" }, 0);
    F ("wahPedal", "Wah pedale", 0.f, 1.f, 0.5f); F ("wahSens", "Wah sensibilite", 0.f, 1.f, 0.5f);
    F ("wahRate", "Wah vitesse", 0.1f, 8.f, 1.f, 0.5f); F ("wahQ", "Wah Q", 1.5f, 12.f, 5.f); F ("wahMix", "Wah mix", 0.f, 1.f, 1.f);
    // ampli
    B ("ampOn", "Ampli on", true); F ("ampGain", "Ampli gain", 0.f, 1.f, 0.2f); F ("bass", "Graves", -1.f, 1.f, 0.f);
    F ("mid", "Mediums", -1.f, 1.f, 0.f); F ("treble", "Aigus", -1.f, 1.f, 0.f); F ("presence", "Presence", -1.f, 1.f, 0.f);
    C ("cab", "Baffle", { "Off", "1x12", "2x12", "4x12" }, 0);
    // tremolo
    B ("tremOn", "Tremolo on", false); F ("tremRate", "Trem vitesse", 0.1f, 12.f, 4.f, 0.5f); F ("tremDepth", "Trem profondeur", 0.f, 1.f, 0.5f);
    C ("tremShape", "Trem forme", { "Sinus", "Triangle", "Carre", "Harmonique" }, 0); F ("tremSpread", "Trem stereo", 0.f, 1.f, 0.3f);
    // phaser
    B ("phOn", "Phaser on", false); F ("phRate", "Phaser vitesse", 0.05f, 5.f, 0.4f, 0.5f); F ("phDepth", "Phaser profondeur", 0.f, 1.f, 0.7f);
    F ("phCentre", "Phaser centre", 200.f, 3000.f, 900.f, 0.5f); F ("phFb", "Phaser feedback", -0.9f, 0.9f, 0.4f); F ("phMix", "Phaser mix", 0.f, 1.f, 0.5f);
    // modulation
    C ("modType", "Modulation", { "Off", "Chorus", "Flanger", "Vibrato", "Leslie" }, 0); F ("modRate", "Mod vitesse", 0.05f, 10.f, 0.8f, 0.5f);
    F ("modDepth", "Mod profondeur", 0.f, 1.f, 0.5f); F ("modFb", "Mod feedback", 0.f, 0.9f, 0.3f); F ("modMix", "Mod mix", 0.f, 1.f, 0.5f);
    B ("lesFast", "Leslie rapide", false);
    // delay
    B ("dlyOn", "Delay on", true); B ("dlySync", "Delay sync", true);
    C ("dlyDiv", "Delay division", { "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2" }, 3);
    F ("dlyMs", "Delay (ms)", 20.f, 1500.f, 380.f, 0.5f); F ("dlyFb", "Delay feedback", 0.f, 0.95f, 0.45f);
    F ("dlyTone", "Delay tone", 300.f, 12000.f, 3500.f, 0.4f); F ("dlyWob", "Delay wobble", 0.f, 1.f, 0.3f);
    F ("dlyWobRate", "Delay vit. wobble", 0.05f, 6.f, 0.5f, 0.5f); F ("dlySpread", "Delay ping-pong", 0.f, 1.f, 0.6f);
    F ("dlySmear", "Delay smear", 0.f, 1.f, 0.3f); F ("dlyDrive", "Delay drive", 0.f, 1.f, 0.25f); F ("dlyMix", "Delay mix", 0.f, 1.f, 0.25f);
    // reverb
    B ("rvOn", "Reverb on", true); C ("rvType", "Reverb type", { "Spring", "Plate", "Hall", "Shimmer" }, 0);
    F ("rvSize", "Reverb taille", 0.f, 1.f, 0.6f); F ("rvDamp", "Reverb damping", 0.f, 1.f, 0.5f);
    F ("rvMix", "Reverb mix", 0.f, 1.f, 0.25f); F ("rvShimmer", "Shimmer", 0.f, 1.f, 0.5f);
    return l;
}

GuitarProcessor::GuitarProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout()), presets (guitarPresets())
{}

void GuitarProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    sr = sampleRate;
    bufSize = 1; while (bufSize < (int) (sr / 25.0) + 16) bufSize <<= 1;
    for (auto& v : voices) { v = Voice(); v.buf.assign ((size_t) bufSize, 0.f); }
    for (auto& p : pend) p.active = false;
    held.clear();
    modDelay.init (sr); leslie.init (sr); spring.init (sr); delay.prepare (sr);
    psDown.init(); psUp.init(); psShim[0].init(); psShim[1].init();
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };
    phaser.prepare (spec); phaser.reset(); reverb.reset();
    rvBuf.setSize (2, blockSize); shimL.assign ((size_t) blockSize, 0.f); shimR.assign ((size_t) blockSize, 0.f);
    for (auto& c : ch) c = Ch();
    for (auto& b : body) b.reset();
    lastBass = -99; lastCab = -1;
    body[0].peak (sr, 110.f, 3.f, 0.f);
}

float GuitarProcessor::noteFreq (int note) const
{
    return 440.f * std::pow (2.f, ((float) note - 69.f + P ("transpose") + P ("fine") / 100.f) / 12.f);
}

int GuitarProcessor::allocVoice()
{
    int best = 0, bestAge = -1;
    for (int i = 0; i < numVoices; ++i)
    {
        if (! voices[i].active) return i;
        if (voices[i].age > bestAge) { bestAge = voices[i].age; best = i; }
    }
    return best;
}

void GuitarProcessor::pluck (int slot, int note, float vel, int owner, float gain)
{
    auto& v = voices[slot];
    v.note = note; v.owner = owner; v.vel = vel; v.released = false; v.active = true; v.age = 0; v.env = 1.f; v.gain = gain;
    v.freq = v.target = noteFreq (note);
    const float Pd = (float) sr / v.freq; const int N = std::max (4, std::min ((int) Pd + 1, bufSize - 8));
    std::fill (v.buf.begin(), v.buf.end(), 0.f);
    std::vector<float> e ((size_t) N), e2 ((size_t) N);
    const float a = 0.1f + P ("pickHard") * 0.9f * (0.6f + 0.4f * vel); float s = 0;
    for (int i = 0; i < N; ++i) { s += a * ((rng.nextFloat() * 2.f - 1.f) - s); e[(size_t) i] = s; }
    const int k = std::max (1, (int) (P ("pickPos") * (float) N)); float mean = 0, pk = 1e-6f;
    for (int i = 0; i < N; ++i) { e2[(size_t) i] = e[(size_t) i] - e[(size_t) ((i + k) % N)]; mean += e2[(size_t) i]; }
    mean /= (float) N;
    for (auto& x : e2) { x -= mean; pk = std::max (pk, std::abs (x)); }
    const float amp = (0.2f + 0.8f * vel) / pk;
    for (int j = 0; j < N; ++j) v.buf[(size_t) ((bufSize - 1 - j) & (bufSize - 1))] = e2[(size_t) j] * amp;
    v.w = 0; v.lp = 0; v.click = 1.f;
    const float pan = juce::jlimit (-1.f, 1.f, ((rng.nextFloat() - 0.5f) + ((float) note - 60.f) / 48.f) * P ("spread"));
    const float ang = (pan + 1.f) * kPi * 0.25f; v.panL = std::cos (ang); v.panR = std::sin (ang);
}

void GuitarProcessor::startNote (int note, float vel, int owner, int delaySamples, float gain)
{
    if (delaySamples <= 0) { pluck (allocVoice(), note, vel, owner, gain); return; }
    for (auto& p : pend) if (! p.active) { p = { true, delaySamples, note, owner, vel, gain }; return; }
}

void GuitarProcessor::noteOn (int key, float vel)
{
    held.erase (std::remove (held.begin(), held.end(), key), held.end()); held.push_back (key);
    if ((int) P ("mode") == 1)
    {
        auto& v = voices[monoVoice];
        if (v.active && ! v.released) { v.target = noteFreq (key); v.owner = key; v.vel = vel; v.note = key; }
        else pluck (monoVoice, key, vel, key, 1.f);
        return;
    }
    auto iv = chords()[(size_t) juce::jlimit (0, (int) chords().size() - 1, (int) P ("chord"))];
    const int dir = (int) P ("strumDir");
    bool down = dir == 0; if (dir == 2) { down = ! strumFlip; strumFlip = ! strumFlip; }
    if (! down) std::reverse (iv.begin(), iv.end());
    const float twelve = P ("twelve"); const int gap = (int) (P ("strumMs") * (float) sr / 1000.f);
    for (size_t i = 0; i < iv.size(); ++i)
    {
        const int d = (int) i * gap, n = key + iv[i]; const float g = 1.f - 0.04f * (float) i;
        startNote (n, vel, key, d, g);
        if (twelve > 0.01f) startNote (n + 12, vel * 0.8f, key, d + (int) (0.001 * sr), twelve * 0.7f);
    }
}

void GuitarProcessor::noteOff (int key)
{
    held.erase (std::remove (held.begin(), held.end(), key), held.end());
    for (auto& p : pend) if (p.active && p.owner == key) p.active = false;
    if ((int) P ("mode") == 1)
    {
        auto& v = voices[monoVoice];
        if (! held.empty()) { v.target = noteFreq (held.back()); v.owner = held.back(); }
        else v.released = true;
        return;
    }
    for (auto& v : voices) if (v.active && v.owner == key) v.released = true;
}

float GuitarProcessor::shape (float p, int type) const
{
    p -= std::floor (p);
    switch (type) { case 1: return 4.f * std::abs (p - 0.5f) - 1.f; case 2: return std::tanh (4.f * std::sin (kTwoPi * p)); default: return std::sin (kTwoPi * p); }
}

void GuitarProcessor::updateTone()
{
    const float b = P ("bass"), m = P ("mid"), t = P ("treble"), pr = P ("presence"); const int cab = (int) P ("cab");
    if (b != lastBass) { for (auto& c : ch) c.bass.lowShelf (sr, 110.f, b * 12.f); lastBass = b; }
    if (m != lastMid)  { for (auto& c : ch) c.mid.peak (sr, 650.f, 0.8f, m * 10.f); lastMid = m; }
    if (t != lastTreb) { for (auto& c : ch) c.treb.highShelf (sr, 3000.f, t * 12.f); lastTreb = t; }
    if (pr != lastPres){ for (auto& c : ch) c.pres.highShelf (sr, 6000.f, pr * 8.f); lastPres = pr; }
    if (cab != lastCab)
    {
        const float hp[4] = { 20.f, 80.f, 70.f, 60.f }, lp[4] = { 20000.f, 5200.f, 5800.f, 6500.f }, pk[4] = { 0.f, 3.f, 2.5f, 3.f }, pf[4] = { 1000.f, 1800.f, 2200.f, 2800.f };
        for (auto& c : ch) { c.cabHp.highpass (sr, hp[cab], 0.7f); c.cabLp1.lowpass (sr, lp[cab], 0.7f); c.cabLp2.lowpass (sr, lp[cab], 0.7f); c.cabPk.peak (sr, pf[cab], 1.f, pk[cab]); }
        lastCab = cab;
    }
}

void GuitarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    auto* oL = buffer.getWritePointer (0); auto* oR = buffer.getWritePointer (1);

    // tempo
    double bpm = 120.0;
    if (auto* ph = getPlayHead()) if (auto pos = ph->getPosition()) if (auto b = pos->getBpm()) bpm = *b;

    // --- parametres de bloc : instrument ---
    const float bendRange = P ("bendRange"), vibRate = P ("vibRate"), vibDepth = P ("vibDepth");
    const float bright = P ("bright"), sustainT = P ("sustain"), sustPlus = P ("sustainPlus"), pickNoise = P ("pickNoise");
    const float vol = P ("vol"), bodyAmt = P ("body");
    const float glide = P ("glide"); glideCoef = glide < 0.002f ? 1.f : 1.f - std::exp (-1.f / (glide * (float) sr));
    for (auto& v : voices)
        if (v.active)
        {
            const float f = std::max (20.f, v.freq);
            float g = std::exp (-6.9078f / (sustainT * f));
            g += (0.99995f - g) * sustPlus * sustPlus;
            v.g = v.released ? g * 0.9f : g;
        }
    const float a0 = (0.1f + 0.88f * std::pow (bright, 1.5f));
    body[0].peak (sr, 105.f, 4.f, bodyAmt * 6.f); body[1].peak (sr, 215.f, 3.f, bodyAmt * 4.f); body[2].peak (sr, 1600.f, 1.5f, bodyAmt * 3.f);

    // --- effets : parametres ---
    const bool compOn = P ("compOn") > 0.5f, octOn = P ("octOn") > 0.5f, drvOn = P ("drvOn") > 0.5f, wahOn = P ("wahOn") > 0.5f,
               ampOn = P ("ampOn") > 0.5f, tremOn = P ("tremOn") > 0.5f, phOn = P ("phOn") > 0.5f, dlyOn = P ("dlyOn") > 0.5f, rvOn = P ("rvOn") > 0.5f;
    const float compThr = P ("compThr"), compRatio = P ("compRatio"), compMk = P ("compMakeup");
    const float compA = std::exp (-1.f / (P ("compAtk") * 0.001f * (float) sr)), compR = std::exp (-1.f / (P ("compRel") * 0.001f * (float) sr));
    const float octD = P ("octDown"), octU = P ("octUp");
    const int dType = (int) P ("drvType"); const float dGainK = P ("drvGain"), dTone = P ("drvTone"), dLevel = P ("drvLevel"), dMix = P ("drvMix");
    static const float gtab[5] = { 5.f, 30.f, 60.f, 90.f, 120.f };
    const float dGain = 1.f + dGainK * gtab[dType];
    const float toneCoef = OnePole::coef (700.f * std::pow (16.f, dTone), sr), muffCoef = OnePole::coef (1300.f, sr), stage1Coef = OnePole::coef (3500.f, sr);
    const int wahMode = (int) P ("wahMode"); const float wahPedal = P ("wahPedal"), wahSens = P ("wahSens"), wahRate = P ("wahRate"), wahQ = P ("wahQ"), wahMix = P ("wahMix");
    const float ampPre = 1.f + P ("ampGain") * 12.f; const int cab = (int) P ("cab");
    updateTone();
    const int tremShape = (int) P ("tremShape"); const float tremRate = P ("tremRate"), tremDepth = P ("tremDepth"), tremSpread = P ("tremSpread");
    const float tremLow = OnePole::coef (700.f, sr);
    const int modType = (int) P ("modType"); const float modRate = P ("modRate"), modDepth = P ("modDepth"), modFb = P ("modFb"), modMix = P ("modMix");
    const bool lesFast = P ("lesFast") > 0.5f;
    const int rvType = (int) P ("rvType"); const float rvSize = P ("rvSize"), rvDamp = P ("rvDamp"), rvMix = P ("rvMix"), rvShim = P ("rvShimmer");
    const double targetDelay = P ("dlySync") > 0.5f ? dlyBeats[juce::jlimit (0, 7, (int) P ("dlyDiv"))] * 60.0 / bpm * sr : P ("dlyMs") * 0.001 * sr;
    Eclipse::PsyDelay::Params dp;
    dp.feedback = P ("dlyFb"); dp.toneHz = P ("dlyTone"); dp.wobble = P ("dlyWob"); dp.wobbleRate = P ("dlyWobRate");
    dp.spread = P ("dlySpread"); dp.smear = P ("dlySmear"); dp.drive = P ("dlyDrive");
    const float dlyMix = P ("dlyMix");
    const int chordMode = (int) P ("mode");
    (void) chordMode;

    auto midiIt = midi.begin();
    for (int i = 0; i < n; ++i)
    {
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i)
        {
            auto m = (*midiIt).getMessage(); ++midiIt;
            if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff()) noteOff (m.getNoteNumber());
            else if (m.isPitchWheel()) bend = ((float) m.getPitchWheelValue() - 8192.f) / 8192.f * bendRange;
            else if (m.isController() && m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.f;
            else if (m.isAllNotesOff() || m.isAllSoundOff()) { for (auto& v : voices) v.released = true; held.clear(); }
        }
        for (auto& p : pend)
            if (p.active && --p.delay <= 0) { p.active = false; pluck (allocVoice(), p.note, p.vel, p.owner, p.gain); }

        vibPh += vibRate / (float) sr; vibPh -= std::floor (vibPh);
        const float semis = bend + std::sin (kTwoPi * vibPh) * (vibDepth + modWheel * 0.6f) * 0.6f;
        const float pitchMul = std::pow (2.f, semis / 12.f);

        float sumL = 0, sumR = 0;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            v.freq += (v.target - v.freq) * glideCoef;
            const float f = v.freq * pitchMul, Pd = (float) sr / f;
            const float a = std::min (1.f, a0 * (0.7f + 0.3f * v.vel)), tau = (1.f - a) / a;
            const float D = juce::jlimit (2.f, (float) bufSize - 4.f, Pd - tau);
            float rp = (float) v.w - D; if (rp < 0) rp += (float) bufSize;
            const int i0 = (int) rp & (bufSize - 1), i1 = (i0 + 1) & (bufSize - 1); const float fr = rp - std::floor (rp);
            const float y = v.buf[(size_t) i0] + (v.buf[(size_t) i1] - v.buf[(size_t) i0]) * fr;
            v.lp += a * (y - v.lp);
            v.buf[(size_t) v.w] = v.lp * v.g; v.w = (v.w + 1) & (bufSize - 1);
            float out = y * v.gain;
            if (v.click > 0.001f) { out += (rng.nextFloat() * 2.f - 1.f) * v.click * pickNoise * 0.3f * v.vel; v.click *= 0.994f; }
            v.env += (std::abs (y) - v.env) * 0.002f; ++v.age;
            if (v.age > 3000 && v.env < 2e-5f) v.active = false;
            sumL += out * v.panL; sumR += out * v.panR;
        }
        // corps de guitare
        float mono = (sumL + sumR) * 0.5f, bd = 0;
        if (bodyAmt > 0.001f) { bd = body[0].process (mono) + body[1].process (mono) + body[2].process (mono); bd = (bd - 3.f * mono) * 0.5f; }
        float l = (sumL + bd) * 0.7f, r = (sumR + bd) * 0.7f;

        // compresseur
        if (compOn)
        {
            const float det = std::max (std::abs (l), std::abs (r));
            compEnv = det > compEnv ? compA * compEnv + (1.f - compA) * det : compR * compEnv + (1.f - compR) * det;
            const float db = 20.f * std::log10 (compEnv + 1e-9f), over = db - compThr;
            const float gr = over > 0 ? over * (1.f - 1.f / compRatio) : 0.f;
            const float gain = std::pow (10.f, (compMk - gr) / 20.f); l *= gain; r *= gain;
        }
        // octaver
        if (octOn)
        {
            const float m = (l + r) * 0.5f;
            const float add = psDown.process (m, 0.5f) * octD + psUp.process (m, 2.f) * octU; l += add; r += add;
        }
        float* io[2] = { &l, &r };
        // wah (position)
        float wahPos = wahPedal;
        if (wahOn)
        {
            const float det = std::abs ((l + r) * 0.5f);
            wahEnv += (det - wahEnv) * (det > wahEnv ? 0.01f : 0.0005f);
            if (wahMode == 1) wahPos = juce::jlimit (0.f, 1.f, wahEnv * wahSens * 8.f);
            else if (wahMode == 2) { wahPh += wahRate / (float) sr; wahPh -= std::floor (wahPh); wahPos = 0.5f + 0.5f * std::sin (kTwoPi * wahPh); }
        }
        const float wahG = wahOn ? tpG (350.f * std::pow (8.f, wahPos), sr) : 0.f;
        for (int c = 0; c < 2; ++c)
        {
            auto& s = ch[c]; float x = *io[c];
            if (drvOn)
            {
                float y = 0;
                switch (dType)
                {
                    case 0: y = std::tanh (x * dGain); break;
                    case 1: y = std::tanh (x * dGain + 0.15f) - std::tanh (0.15f); break;
                    case 2: { const float z = x * dGain; y = z >= 0 ? 1.f - std::exp (-z) : -(1.f - std::exp (z * 0.6f)) * 0.9f; break; }
                    case 3: { float z = std::tanh (x * dGain); z = s.dlp1.lp (z, stage1Coef); y = std::tanh (z * (1.f + dGainK * 8.f)); break; }
                    default: y = std::tanh (juce::jlimit (-0.5f, 0.5f, x * dGain) * 3.f); break;
                }
                if (dType == 3) { const float lo = s.dlpT.lp (y, muffCoef), hi = y - lo; y = lo * (1.f - dTone) * 1.3f + hi * dTone * 0.9f; }
                else y = s.dlpT.lp (y, toneCoef);
                y *= dLevel * 0.6f; x = x * (1.f - dMix) + y * dMix;
            }
            if (wahOn)
            {
                const float k = 1.f / wahQ; s.wah.process (x, wahG, k);
                x = x * (1.f - wahMix) + s.wah.bp * k * 2.2f * wahMix;
            }
            if (ampOn)
            {
                x = std::tanh (x * ampPre) / std::sqrt (std::min (ampPre, 4.f));
                x = s.pres.process (s.treb.process (s.mid.process (s.bass.process (x))));
                if (cab > 0) x = s.cabLp2.process (s.cabLp1.process (s.cabPk.process (s.cabHp.process (x))));
            }
            *io[c] = x;
        }
        // tremolo
        if (tremOn)
        {
            tremPh += tremRate / (float) sr; tremPh -= std::floor (tremPh);
            const float off = tremSpread * 0.5f;
            if (tremShape == 3)
            {
                const float s0 = std::sin (kTwoPi * tremPh), s1 = std::sin (kTwoPi * (tremPh + off));
                const float loL = ch[0].tremLp.lp (l, tremLow), loR = ch[1].tremLp.lp (r, tremLow);
                l = loL * (1.f - tremDepth * (0.5f + 0.5f * s0)) + (l - loL) * (1.f - tremDepth * (0.5f - 0.5f * s0));
                r = loR * (1.f - tremDepth * (0.5f + 0.5f * s1)) + (r - loR) * (1.f - tremDepth * (0.5f - 0.5f * s1));
            }
            else
            {
                l *= 1.f - tremDepth * (0.5f + 0.5f * shape (tremPh, tremShape));
                r *= 1.f - tremDepth * (0.5f + 0.5f * shape (tremPh + off, tremShape));
            }
        }
        oL[i] = l; oR[i] = r;
    }

    // phaser (bloc)
    if (phOn)
    {
        phaser.setRate (P ("phRate")); phaser.setDepth (P ("phDepth")); phaser.setCentreFrequency (P ("phCentre"));
        phaser.setFeedback (P ("phFb")); phaser.setMix (P ("phMix"));
        juce::dsp::AudioBlock<float> blk (buffer); phaser.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }

    // modulation, delay, reverb ressort
    for (int i = 0; i < n; ++i)
    {
        float l = oL[i], r = oR[i];
        switch (modType)
        {
            case 1: modDelay.process (l, r, modRate, modDepth * 6.f, 15.f, 0.f, modMix); break;
            case 2: modDelay.process (l, r, modRate, modDepth * 2.2f, 2.8f, modFb, modMix); break;
            case 3: modDelay.process (l, r, modRate, modDepth * 3.f, 4.f, 0.f, 1.f); break;
            case 4: leslie.process (l, r, lesFast, modDepth, 0.5f, modMix); break;
            default: break;
        }
        if (dlyOn)
        {
            delayTime += (targetDelay - delayTime) * 0.0004; dp.timeSamples = (float) delayTime;
            float yl, yr; delay.process (l, r, yl, yr, dp); l += yl * dlyMix; r += yr * dlyMix;
        }
        if (rvOn && rvType == 0)
        {
            float wl, wr; spring.process ((l + r) * 0.5f, 0.3f + 0.62f * rvSize, rvDamp, wl, wr);
            l += wl * rvMix * 0.8f; r += wr * rvMix * 0.8f;
        }
        oL[i] = l; oR[i] = r;
    }
    if (rvOn && rvType > 0)
    {
        auto* rl = rvBuf.getWritePointer (0); auto* rr = rvBuf.getWritePointer (1);
        const float shim = rvType == 3 ? rvShim * 0.5f : 0.f;
        for (int i = 0; i < n; ++i) { rl[i] = oL[i] * 0.5f + shimL[(size_t) i]; rr[i] = oR[i] * 0.5f + shimR[(size_t) i]; }
        juce::Reverb::Parameters rp;
        rp.roomSize = rvType == 1 ? 0.4f + 0.45f * rvSize : 0.7f + 0.29f * rvSize; rp.damping = rvDamp * (rvType == 1 ? 0.6f : 1.f);
        rp.wetLevel = 1.f; rp.dryLevel = 0.f; rp.width = 1.f; rp.freezeMode = 0.f; reverb.setParameters (rp);
        reverb.processStereo (rl, rr, n);
        for (int i = 0; i < n; ++i)
        {
            oL[i] += rl[i] * rvMix * 1.4f; oR[i] += rr[i] * rvMix * 1.4f;
            shimL[(size_t) i] = psShim[0].process (rl[i], 2.f) * shim; shimR[(size_t) i] = psShim[1].process (rr[i], 2.f) * shim;
        }
    }
    else { std::fill (shimL.begin(), shimL.end(), 0.f); std::fill (shimR.begin(), shimR.end(), 0.f); }
    for (int i = 0; i < n; ++i) { oL[i] = std::tanh (oL[i] * vol); oR[i] = std::tanh (oR[i] * vol); }
}

juce::AudioProcessorEditor* GuitarProcessor::createEditor() { return new GuitarEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GuitarProcessor(); }
