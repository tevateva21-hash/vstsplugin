#include "KinoProcessor.h"
#include "KinoEditor.h"

namespace
{
const int kScales[10][12] = {
    { 0, 1, 4, 5, 7, 8, 10, -1 },                 // 0: Phrygian Dominant (7)
    { 0, 2, 3, 5, 7, 8, 11, -1 },                 // 1: Harmonic Minor (7)
    { 0, 2, 3, 5, 7, 8, 10, -1 },                 // 2: Natural Minor (7)
    { 0, 2, 3, 5, 7, 9, 10, -1 },                 // 3: Dorian (7)
    { 0, 3, 5, 7, 10, -1 },                       // 4: Minor Pentatonic (5)
    { 0, 1, 3, 5, 7, 8, 10, -1 },                 // 5: Phrygian (7)
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },     // 6: Chromatic (12)
    { 0, 2, 3, 7, 8, -1 },                        // 7: Hirajoshi (5)
    { 0, 2, 4, 6, 8, 10, -1 },                    // 8: Whole Tone (6)
    { 0, 2, 4, 5, 7, 9, 11, -1 }                  // 9: Major (7)
};

int scaleLen (int s)
{
    if (s == 6) return 12;
    for (int i = 0; i < 12; ++i)
        if (kScales[s][i] < 0) return i;
    return 7;
}

const char* kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

KinoProcessor::KinoProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KinoState", createLayout())
{
    initModels();
    loadModel (0);
}

juce::AudioProcessorValueTreeState::ParameterLayout KinoProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto F = [&] (const char* id, const char* nm, float mn, float mx, float df, float st = 0.0f)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (id, nm, juce::NormalisableRange<float> (mn, mx, st), df));
    };
    auto I = [&] (const char* id, const char* nm, int mn, int mx, int df)
    {
        p.push_back (std::make_unique<juce::AudioParameterInt> (id, nm, mn, mx, df));
    };
    auto B = [&] (const char* id, const char* nm, bool df)
    {
        p.push_back (std::make_unique<juce::AudioParameterBool> (id, nm, df));
    };
    auto C = [&] (const char* id, const char* nm, juce::StringArray ch, int df)
    {
        p.push_back (std::make_unique<juce::AudioParameterChoice> (id, nm, ch, df));
    };

    B ("run", "PLAY", true);
    F ("bpm", "BPM", 60.0f, 200.0f, 138.0f, 0.5f);
    C ("root", "ROOT", { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 6);
    I ("octave", "OCTAVE", 1, 6, 2);
    C ("scale", "GAMME", { "Phrygian Dom", "Harmonic Min", "Natural Min", "Dorian", "Pentatonic Min", "Phrygian", "Chromatic", "Hirajoshi", "Whole Tone", "Major" }, 0);
    I ("steps", "STEPS", 4, 32, 16);
    C ("rate", "RATE", { "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" }, 2);
    F ("swing", "SWING", 0.0f, 0.45f, 0.0f, 0.01f);
    F ("intensity", "INTENSITE", 0.1f, 1.0f, 0.85f, 0.01f);
    C ("intCurve", "COURBE INT", { "Flat", "Crescendo", "Decrescendo", "Pulse", "Random" }, 0);
    C ("chordMode", "ACCORDS", { "Single", "Octave +1", "Fifth", "Minor Triad", "Sus4", "7th" }, 0);
    I ("eucPulses", "EUCLID PULSES", 1, 32, 8);
    I ("eucRot", "EUCLID ROT", 0, 31, 0);
    I ("midiCh", "MIDI CH", 1, 16, 1);
    B ("liveTrans", "MIDI TRANSPOSE", false);

    // Saw Wave basique pour pre-ecoute interne
    B ("sawOn", "SAW ON", true);
    F ("sawVol", "SAW VOL", 0.0f, 1.0f, 0.45f, 0.01f);
    F ("sawCut", "SAW CUTOFF", 150.0f, 12000.0f, 2600.0f, 1.0f);
    F ("sawDecay", "SAW DECAY", 0.03f, 1.2f, 0.20f, 0.01f);

    return { p.begin(), p.end() };
}

bool KinoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void KinoProcessor::prepareToPlay (double sampleRate, int)
{
    sr = std::max (8000.0, sampleRate);
    pendingOffs.clear();
    for (auto& v : voices) v.active = false;
}

KinoStep KinoProcessor::getStep (int idx) const
{
    const juce::SpinLock::ScopedLockType sl (stepLock);
    return steps[(size_t) juce::jlimit (0, maxSteps - 1, idx)];
}

void KinoProcessor::setStep (int idx, const KinoStep& s)
{
    const juce::SpinLock::ScopedLockType sl (stepLock);
    steps[(size_t) juce::jlimit (0, maxSteps - 1, idx)] = s;
}

void KinoProcessor::toggleCell (int step, int degree)
{
    if (step < 0 || step >= maxSteps) return;
    const juce::SpinLock::ScopedLockType sl (stepLock);
    auto& s = steps[(size_t) step];
    if (s.active && s.degree == degree)
        s.active = false;
    else
    {
        s.active = true;
        s.degree = degree;
    }
}

int KinoProcessor::degreeToMidiNote (int degree, int octJump) const
{
    const int root = (int) P ("root");
    const int baseOct = (int) P ("octave") + (hasLiveTranspose ? liveMidiTranspose : 0) + octJump;
    const int sc = juce::jlimit (0, 9, (int) P ("scale"));
    const int len = scaleLen (sc);

    int octShift = 0;
    int deg = degree;
    while (deg < 0) { deg += len; --octShift; }
    while (deg >= len) { deg -= len; ++octShift; }

    const int semi = kScales[sc][deg];
    const int note = 12 * (baseOct + 1 + octShift) + root + semi;
    return juce::jlimit (0, 127, note);
}

juce::String KinoProcessor::noteNameForDegree (int degree) const
{
    const int n = degreeToMidiNote (degree, 0);
    const int oct = (n / 12) - 1;
    return juce::String (kNoteNames[n % 12]) + juce::String (oct);
}

float KinoProcessor::intensityMultiplierForStep (int step, int totalSteps) const
{
    const int curve = (int) P ("intCurve");
    const float globalInt = P ("intensity");
    const float u = totalSteps > 1 ? (float) step / (float) (totalSteps - 1) : 1.0f;

    float c = 1.0f;
    switch (curve)
    {
        case 0: c = 1.0f; break;                                   // Flat
        case 1: c = 0.25f + 0.75f * u * u; break;                  // Crescendo (Snare Roll!)
        case 2: c = 1.0f - 0.72f * u; break;                       // Decrescendo
        case 3: c = 0.45f + 0.55f * (0.5f + 0.5f * std::sin (u * juce::MathConstants<float>::twoPi * 2.0f)); break; // Pulse
        case 4: c = 0.5f + 0.5f * ((float) ((step * 37 + 13) % 100) / 100.0f); break; // Deterministic random
        default: break;
    }
    return juce::jlimit (0.1f, 1.25f, c * globalInt);
}

void KinoProcessor::triggerNoteOn (juce::MidiBuffer& out, int sampleOffset, int note, int vel, int ch, double durSamples, bool slide, bool accent)
{
    note = juce::jlimit (0, 127, note);
    vel = juce::jlimit (1, 127, vel);
    out.addEvent (juce::MidiMessage::noteOn (ch, note, (juce::uint8) vel), sampleOffset);
    pendingOffs.push_back ({ note, ch, durSamples * (slide ? 1.18 : 1.0) });

    // Trigger internal Saw preview voice
    if (P ("sawOn") > 0.5f)
    {
        int slot = 0;
        float minEnv = 999.0f;
        for (int i = 0; i < (int) voices.size(); ++i)
        {
            if (! voices[(size_t) i].active) { slot = i; break; }
            if (voices[(size_t) i].env < minEnv) { minEnv = voices[(size_t) i].env; slot = i; }
        }
        auto& v = voices[(size_t) slot];
        const double f = 440.0 * std::pow (2.0, (note - 69) / 12.0);
        if (! v.active || ! slide) v.freq = f;
        v.targetFreq = f;
        v.note = note;
        v.active = true;
        v.env = 1.0f;
        v.vel = (float) vel / 127.0f;
        v.accent = accent;
    }
}

void KinoProcessor::processInternalSaw (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi)
{
    if (P ("sawOn") < 0.5f)
    {
        buffer.clear();
        return;
    }

    const int nSamp = buffer.getNumSamples();
    const float vol = P ("sawVol") * 0.35f;
    const float baseCut = P ("sawCut");
    const float decaySec = std::max (0.02f, P ("sawDecay"));
    const float envDecayCoeff = std::exp (-1.0f / (float) (sr * decaySec));

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())
        {
            // Only trigger from keyboard if not already triggered by sequencer
            bool found = false;
            for (auto& v : voices)
                if (v.active && v.note == m.getNoteNumber() && v.env > 0.95f) { found = true; break; }
            if (! found)
            {
                int slot = 0;
                for (int i = 0; i < (int) voices.size(); ++i)
                    if (! voices[(size_t) i].active) { slot = i; break; }
                auto& v = voices[(size_t) slot];
                v.active = true;
                v.note = m.getNoteNumber();
                v.freq = v.targetFreq = 440.0 * std::pow (2.0, (v.note - 69) / 12.0);
                v.env = 1.0f;
                v.vel = m.getFloatVelocity();
                v.accent = m.getVelocity() > 110;
            }
        }
    }

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int i = 0; i < nSamp; ++i)
    {
        float sum = 0.0f;
        for (auto& v : voices)
        {
            if (! v.active) continue;
            v.freq += (v.targetFreq - v.freq) * 0.005; // smooth slide
            v.phase += v.freq / sr;
            if (v.phase >= 1.0) v.phase -= 1.0;

            const float saw = (float) (2.0 * v.phase - 1.0);
            const float cut = juce::jlimit (80.0f, (float) (sr * 0.42), baseCut * (0.25f + 0.75f * v.env) * (v.accent ? 1.5f : 1.0f));
            const float alpha = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * cut / (float) sr);
            v.lpState += alpha * (saw - v.lpState);

            sum += v.lpState * v.env * v.vel;
            v.env *= envDecayCoeff;
            if (v.env < 0.002f) v.active = false;
        }
        const float out = std::tanh (sum * vol);
        L[i] = out;
        if (R) R[i] = out;
    }
}

void KinoProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    // Merge virtual keyboard + incoming MIDI
    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

    const bool liveTrans = P ("liveTrans") > 0.5f;
    for (const auto meta : midiMessages)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn() && liveTrans)
        {
            const int rootMidi = 12 * ((int) P ("octave") + 1) + (int) P ("root");
            liveMidiTranspose = (m.getNoteNumber() - rootMidi) / 12;
            hasLiveTranspose = true;
        }
    }

    // Clear incoming MIDI if we only use it to drive/transpose, or pass through?
    // Keep external MIDI only if not liveTrans, otherwise Kino generates clean MIDI out.
    if (liveTrans) midiMessages.clear();

    const int nSamp = buffer.getNumSamples();
    const bool running = P ("run") > 0.5f;
    double bpm = (double) P ("bpm");
    bool hostPlaying = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (pos->getIsPlaying()) hostPlaying = true;
        }
    }

    // Process pending note-offs
    for (int i = (int) pendingOffs.size() - 1; i >= 0; --i)
    {
        auto& po = pendingOffs[(size_t) i];
        if (po.samplesRemaining <= (double) nSamp)
        {
            const int off = juce::jlimit (0, nSamp - 1, (int) po.samplesRemaining);
            midiMessages.addEvent (juce::MidiMessage::noteOff (po.channel, po.note), off);
            pendingOffs.erase (pendingOffs.begin() + i);
        }
        else
        {
            po.samplesRemaining -= (double) nSamp;
        }
    }

    const float vp = visualPulse.load();
    visualPulse.store (std::max (0.0f, vp - (float) nSamp / (float) sr * 4.5f));

    if (! running && ! hostPlaying)
    {
        processInternalSaw (buffer, midiMessages);
        return;
    }

    static constexpr double kStepBeats[6] = { 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
    const int rateIdx = juce::jlimit (0, 5, (int) P ("rate"));
    const double stepSamples = std::max (64.0, (60.0 / bpm) * kStepBeats[rateIdx] * sr);
    const int numSteps = juce::jlimit (4, maxSteps, (int) P ("steps"));
    const float swing = P ("swing");
    const int ch = juce::jlimit (1, 16, (int) P ("midiCh"));
    const int chordMode = (int) P ("chordMode");

    for (int s = 0; s < nSamp; ++s)
    {
        const double swingOffset = (seqStep % 2 == 1) ? swing * stepSamples : 0.0;
        const double effectiveLen = stepSamples + ((seqStep % 2 == 0) ? swing * stepSamples : -swing * stepSamples);

        const KinoStep st = getStep (seqStep);
        const int rCount = std::max (1, st.ratchet);
        const double subStepLen = std::max (32.0, effectiveLen / (double) rCount);

        const double localPhase = stepPhase - swingOffset;
        if (localPhase >= 0.0)
        {
            const int subIdx = std::min (rCount - 1, (int) (localPhase / subStepLen));
            if (subIdx != lastRatchetIdx)
            {
                lastRatchetIdx = subIdx;
                if (st.active && rng.nextInt (100) < st.prob)
                {
                    const float mult = intensityMultiplierForStep (seqStep, numSteps);
                    // Subtle ramp inside ratchets if crescendo is active
                    const float rRamp = (P ("intCurve") == 1 && rCount > 1) ? (0.7f + 0.3f * ((float) subIdx / (float) (rCount - 1))) : 1.0f;
                    const int vel = juce::jlimit (1, 127, (int) std::round ((float) st.velocity * mult * rRamp * (st.accent ? 1.25f : 1.0f)));
                    const int baseNote = degreeToMidiNote (st.degree, st.octJump);
                    const double dur = subStepLen * (double) juce::jlimit (0.05f, 1.0f, st.gate);

                    triggerNoteOn (midiMessages, s, baseNote, vel, ch, dur, st.slide, st.accent);

                    // Chord modes
                    if (chordMode == 1) triggerNoteOn (midiMessages, s, baseNote + 12, (int) (vel * 0.85f), ch, dur, st.slide, st.accent);
                    else if (chordMode == 2) triggerNoteOn (midiMessages, s, baseNote + 7, (int) (vel * 0.85f), ch, dur, st.slide, st.accent);
                    else if (chordMode == 3)
                    {
                        triggerNoteOn (midiMessages, s, baseNote + 3, (int) (vel * 0.85f), ch, dur, st.slide, st.accent);
                        triggerNoteOn (midiMessages, s, baseNote + 7, (int) (vel * 0.8f), ch, dur, st.slide, st.accent);
                    }
                    else if (chordMode == 4)
                    {
                        triggerNoteOn (midiMessages, s, baseNote + 5, (int) (vel * 0.85f), ch, dur, st.slide, st.accent);
                        triggerNoteOn (midiMessages, s, baseNote + 7, (int) (vel * 0.8f), ch, dur, st.slide, st.accent);
                    }
                    else if (chordMode == 5)
                    {
                        triggerNoteOn (midiMessages, s, baseNote + 3, (int) (vel * 0.85f), ch, dur, st.slide, st.accent);
                        triggerNoteOn (midiMessages, s, baseNote + 7, (int) (vel * 0.8f), ch, dur, st.slide, st.accent);
                        triggerNoteOn (midiMessages, s, baseNote + 10, (int) (vel * 0.75f), ch, dur, st.slide, st.accent);
                    }

                    visualPulse.store (std::min (1.0f, (float) vel / 127.0f));
                }
            }
        }

        stepPhase += 1.0;
        if (stepPhase >= effectiveLen)
        {
            stepPhase -= effectiveLen;
            seqStep = (seqStep + 1) % numSteps;
            currentStep.store (seqStep);
            lastRatchetIdx = -1;
        }
    }

    processInternalSaw (buffer, midiMessages);
}

void KinoProcessor::initModels()
{
    models.clear();
    auto makeStep = [] (bool act, int deg, int vel = 100, float gate = 0.65f, int rat = 1, bool sl = false, bool acc = false, int prob = 100, int oct = 0)
    {
        return KinoStep { act, deg, vel, gate, rat, sl, acc, prob, oct };
    };

    // 1. Psy Rolling 16th (Bassline)
    {
        KinoModel m; m.category = "Bassline"; m.name = "Psy Rolling 16th"; m.root = 6; m.octave = 2; m.scale = 0; m.steps = 16; m.rate = 2;
        for (int i = 0; i < 16; ++i)
        {
            bool act = (i % 4 != 0);
            int vel = (i % 4 == 2) ? 115 : 88;
            bool acc = (i % 4 == 2);
            int deg = (i == 15) ? 7 : 0;
            m.pattern.push_back (makeStep (act, deg, vel, 0.55f, 1, false, acc));
        }
        models.push_back (m);
    }

    // 2. Offbeat Gallop (Bassline)
    {
        KinoModel m; m.category = "Bassline"; m.name = "Offbeat Gallop"; m.root = 5; m.octave = 2; m.scale = 2; m.steps = 16; m.rate = 2;
        for (int i = 0; i < 16; ++i)
        {
            bool act = (i % 4 == 2 || i % 4 == 3);
            int deg = (i >= 12 && i % 4 == 3) ? 2 : 0;
            m.pattern.push_back (makeStep (act, deg, i % 4 == 2 ? 120 : 85, 0.6f, 1, false, i % 4 == 2));
        }
        models.push_back (m);
    }

    // 3. Dark Forest Triplet (Bassline)
    {
        KinoModel m; m.category = "Bassline"; m.name = "Dark Forest Triplet"; m.root = 4; m.octave = 2; m.scale = 5; m.steps = 12; m.rate = 5;
        for (int i = 0; i < 12; ++i)
        {
            bool act = (i % 3 != 0);
            int deg = (i == 11) ? 1 : 0;
            m.pattern.push_back (makeStep (act, deg, i % 3 == 2 ? 110 : 82, 0.52f));
        }
        models.push_back (m);
    }

    // 4. Acid Squelch 303
    {
        KinoModel m; m.category = "Acid 303"; m.name = "Acid Squelch 303"; m.root = 6; m.octave = 2; m.scale = 0; m.steps = 16; m.rate = 2;
        const int degs[16] = { 0, 0, 7, 0, 1, 0, 4, 7, 0, 3, 0, 7, 1, 4, 7, 0 };
        const int act[16]  = { 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1 };
        const int sl[16]   = { 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0 };
        const int acc[16]  = { 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0 };
        for (int i = 0; i < 16; ++i)
            m.pattern.push_back (makeStep (act[i] > 0, degs[i], acc[i] ? 120 : 90, 0.68f, 1, sl[i] > 0, acc[i] > 0));
        models.push_back (m);
    }

    // 5. Goa Acid Screamer 32
    {
        KinoModel m; m.category = "Acid 303"; m.name = "Goa Acid Screamer 32"; m.root = 7; m.octave = 2; m.scale = 0; m.steps = 32; m.rate = 2;
        for (int i = 0; i < 32; ++i)
        {
            bool a = (i % 2 == 0) || (i % 7 == 0) || (i > 24);
            int d = (i % 8 == 0) ? 0 : ((i * 3) % 8);
            bool sl = (i % 5 == 2);
            bool ac = (i % 4 == 0 || i == 30);
            int rat = (i == 31) ? 2 : 1;
            m.pattern.push_back (makeStep (a, d, ac ? 122 : 88, 0.65f, rat, sl, ac));
        }
        models.push_back (m);
    }

    // 6. Psy Arp Rising
    {
        KinoModel m; m.category = "Arps & Leads"; m.name = "Psy Arp Rising"; m.root = 9; m.octave = 3; m.scale = 1; m.steps = 16; m.rate = 2;
        const int degs[16] = { 0, 2, 4, 7, 2, 4, 7, 9, 4, 7, 9, 11, 7, 4, 2, 0 };
        for (int i = 0; i < 16; ++i)
            m.pattern.push_back (makeStep (true, degs[i], (i % 4 == 0) ? 115 : 90, 0.55f, 1, false, i % 4 == 0));
        models.push_back (m);
    }

    // 7. Tribal Hirajoshi Arp
    {
        KinoModel m; m.category = "Arps & Leads"; m.name = "Tribal Hirajoshi Arp"; m.root = 2; m.octave = 3; m.scale = 7; m.steps = 16; m.rate = 2;
        const int degs[16] = { 0, 2, 4, 5, 4, 2, 0, 5, 2, 4, 7, 5, 4, 2, 1, 0 };
        for (int i = 0; i < 16; ++i)
            m.pattern.push_back (makeStep (i % 5 != 3, degs[i], 96, 0.6f, (i == 14) ? 2 : 1, false, i % 4 == 0, 90));
        models.push_back (m);
    }

    // 8. Dub Techno Stab (Chords)
    {
        KinoModel m; m.category = "Chords & Stabs"; m.name = "Dub Techno Stab"; m.root = 5; m.octave = 3; m.scale = 2; m.steps = 16; m.rate = 2; m.chordMode = 3;
        for (int i = 0; i < 16; ++i)
        {
            bool a = (i == 3 || i == 6 || i == 11 || i == 14);
            int d = (i >= 11) ? 2 : 0;
            m.pattern.push_back (makeStep (a, d, a ? 108 : 80, 0.35f, 1, false, i == 3));
        }
        models.push_back (m);
    }

    // 9. Melodic Prog Chords
    {
        KinoModel m; m.category = "Chords & Stabs"; m.name = "Melodic Prog Chords"; m.root = 0; m.octave = 3; m.scale = 3; m.steps = 16; m.rate = 1; m.chordMode = 5;
        for (int i = 0; i < 16; ++i)
        {
            bool a = (i % 4 == 0 || i == 14);
            int d = (i < 4) ? 0 : (i < 8 ? 3 : (i < 12 ? 4 : 2));
            m.pattern.push_back (makeStep (a, d, 95, 0.85f));
        }
        models.push_back (m);
    }

    // 10. Snare Roll Crescendo (with Intensity Automation!)
    {
        KinoModel m; m.category = "Snare Rolls & Fills"; m.name = "Snare Roll Crescendo"; m.root = 2; m.octave = 3; m.scale = 6; m.steps = 16; m.rate = 2; m.intCurve = 1;
        for (int i = 0; i < 16; ++i)
        {
            int rat = (i >= 14) ? 4 : (i >= 12 ? 2 : 1);
            int vel = 60 + i * 4;
            m.pattern.push_back (makeStep (true, 0, vel, 0.45f, rat, false, i >= 12));
        }
        models.push_back (m);
    }

    // 11. Build-Up 32nd Snare
    {
        KinoModel m; m.category = "Snare Rolls & Fills"; m.name = "Build-Up 32nd Snare"; m.root = 2; m.octave = 3; m.scale = 6; m.steps = 32; m.rate = 3; m.intCurve = 1;
        for (int i = 0; i < 32; ++i)
        {
            int deg = (i >= 24) ? (i - 24) / 2 : 0; // rising pitch at end!
            int rat = (i >= 28) ? 2 : 1;
            m.pattern.push_back (makeStep (true, deg, 110, 0.4f, rat, false, i >= 24));
        }
        models.push_back (m);
    }

    // 12. Techno Snare & Clap
    {
        KinoModel m; m.category = "Percussions"; m.name = "Techno Snare & Clap"; m.root = 2; m.octave = 3; m.scale = 6; m.steps = 16; m.rate = 2;
        for (int i = 0; i < 16; ++i)
        {
            bool a = (i == 4 || i == 12 || i == 7 || i == 15);
            int vel = (i == 4 || i == 12) ? 120 : 65;
            int deg = (i == 4 || i == 12) ? 1 : 0;
            m.pattern.push_back (makeStep (a, deg, vel, 0.45f, (i == 15) ? 2 : 1, false, i == 4 || i == 12));
        }
        models.push_back (m);
    }

    // 13. Clap Groove Euclid
    {
        KinoModel m; m.category = "Percussions"; m.name = "Clap Groove Euclid"; m.root = 3; m.octave = 3; m.scale = 6; m.steps = 16; m.rate = 2;
        for (int i = 0; i < 16; ++i)
        {
            bool a = (i == 4 || i == 10 || i == 12 || i == 14);
            m.pattern.push_back (makeStep (a, 0, a && i % 4 == 0 ? 115 : 85, 0.4f, 1, false, i == 4 || i == 12, i == 14 ? 75 : 100));
        }
        models.push_back (m);
    }

    // 14. Breakbeat Fill 16
    {
        KinoModel m; m.category = "Snare Rolls & Fills"; m.name = "Breakbeat Fill 16"; m.root = 0; m.octave = 3; m.scale = 4; m.steps = 16; m.rate = 2; m.intCurve = 1;
        const int act[16]  = { 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1 };
        const int degs[16] = { 0, 0, 2, 0, 4, 2, 0, 3, 2, 0, 4, 3, 2, 4, 5, 7 };
        const int rats[16] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 2, 3, 4 };
        for (int i = 0; i < 16; ++i)
            m.pattern.push_back (makeStep (act[i] > 0, degs[i], 90 + i * 2, 0.5f, rats[i], false, i >= 12));
        models.push_back (m);
    }
}

void KinoProcessor::loadModel (int index)
{
    if (models.empty()) return;
    const int idx = juce::jlimit (0, (int) models.size() - 1, index);
    currentModelIndex.store (idx);
    const auto& m = models[(size_t) idx];

    auto setParam = [&] (const char* id, float val)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (val));
    };

    setParam ("root", (float) m.root);
    setParam ("octave", (float) m.octave);
    setParam ("scale", (float) m.scale);
    setParam ("steps", (float) m.steps);
    setParam ("rate", (float) m.rate);
    setParam ("intCurve", (float) m.intCurve);
    setParam ("chordMode", (float) m.chordMode);
    setParam ("swing", m.swing);

    const juce::SpinLock::ScopedLockType sl (stepLock);
    for (int i = 0; i < maxSteps; ++i)
    {
        if (i < (int) m.pattern.size())
            steps[(size_t) i] = m.pattern[(size_t) i];
        else if (! m.pattern.empty())
            steps[(size_t) i] = m.pattern[(size_t) (i % (int) m.pattern.size())];
        else
            steps[(size_t) i] = KinoStep {};
    }
}

void KinoProcessor::applyEuclidean (int pulses, int rotation)
{
    const int numSteps = juce::jlimit (4, maxSteps, (int) P ("steps"));
    pulses = juce::jlimit (1, numSteps, pulses);
    rotation = ((rotation % numSteps) + numSteps) % numSteps;

    const juce::SpinLock::ScopedLockType sl (stepLock);
    for (int i = 0; i < numSteps; ++i)
    {
        const int idx = (i + rotation) % numSteps;
        const bool hit = (((i * pulses) % numSteps) < pulses);
        steps[(size_t) idx].active = hit;
        if (hit && steps[(size_t) idx].velocity < 50)
            steps[(size_t) idx].velocity = 100;
    }
}

void KinoProcessor::mutatePattern()
{
    const int numSteps = juce::jlimit (4, maxSteps, (int) P ("steps"));
    const juce::SpinLock::ScopedLockType sl (stepLock);
    for (int i = 0; i < numSteps; ++i)
    {
        auto& s = steps[(size_t) i];
        const int r = rng.nextInt (100);
        if (r < 12) s.active = ! s.active;
        if (s.active)
        {
            if (rng.nextInt (100) < 35)
                s.degree = juce::jlimit (-5, 12, s.degree + rng.nextInt ({ -2, 3 }));
            if (rng.nextInt (100) < 30)
                s.velocity = juce::jlimit (45, 127, s.velocity + rng.nextInt ({ -20, 21 }));
            if (rng.nextInt (100) < 15)
                s.ratchet = rng.nextInt (100) < 70 ? 1 : 2;
            if (rng.nextInt (100) < 15)
                s.slide = ! s.slide;
        }
    }
}

void KinoProcessor::clearPattern()
{
    const juce::SpinLock::ScopedLockType sl (stepLock);
    for (auto& s : steps) s = KinoStep {};
}

bool KinoProcessor::exportMidiFile (const juce::File& file) const
{
    juce::MidiFile mf;
    const short tpq = 480;
    mf.setTicksPerQuarterNote (tpq);

    juce::MidiMessageSequence seq;
    const double bpm = (double) P ("bpm");
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)), 0.0);

    static constexpr double kStepBeats[6] = { 1.0, 0.5, 0.25, 0.125, 1.0 / 3.0, 1.0 / 6.0 };
    const int rateIdx = juce::jlimit (0, 5, (int) P ("rate"));
    const double stepTicks = (double) tpq * kStepBeats[rateIdx];
    const int numSteps = juce::jlimit (4, maxSteps, (int) P ("steps"));
    const float swing = P ("swing");
    const int ch = juce::jlimit (1, 16, (int) P ("midiCh"));
    const int chordMode = (int) P ("chordMode");

    double curTick = 0.0;
    for (int i = 0; i < numSteps; ++i)
    {
        const double effLen = stepTicks + ((i % 2 == 0) ? swing * stepTicks : -swing * stepTicks);
        const KinoStep st = getStep (i);
        if (st.active)
        {
            const int rCount = std::max (1, st.ratchet);
            const double subLen = effLen / (double) rCount;
            const float mult = intensityMultiplierForStep (i, numSteps);
            for (int r = 0; r < rCount; ++r)
            {
                const int vel = juce::jlimit (1, 127, (int) std::round ((float) st.velocity * mult * (st.accent ? 1.25f : 1.0f)));
                const int baseNote = degreeToMidiNote (st.degree, st.octJump);
                const double tOn = curTick + r * subLen;
                const double tOff = tOn + subLen * (double) juce::jlimit (0.05f, 1.0f, st.gate) * (st.slide ? 1.15 : 1.0);

                auto addNote = [&] (int n, int v)
                {
                    n = juce::jlimit (0, 127, n);
                    seq.addEvent (juce::MidiMessage::noteOn (ch, n, (juce::uint8) v), tOn);
                    seq.addEvent (juce::MidiMessage::noteOff (ch, n), tOff);
                };

                addNote (baseNote, vel);
                if (chordMode == 1) addNote (baseNote + 12, (int) (vel * 0.85f));
                else if (chordMode == 2) addNote (baseNote + 7, (int) (vel * 0.85f));
                else if (chordMode == 3) { addNote (baseNote + 3, (int) (vel * 0.85f)); addNote (baseNote + 7, (int) (vel * 0.8f)); }
                else if (chordMode == 4) { addNote (baseNote + 5, (int) (vel * 0.85f)); addNote (baseNote + 7, (int) (vel * 0.8f)); }
                else if (chordMode == 5) { addNote (baseNote + 3, (int) (vel * 0.85f)); addNote (baseNote + 7, (int) (vel * 0.8f)); addNote (baseNote + 10, (int) (vel * 0.75f)); }
            }
        }
        curTick += effLen;
    }

    seq.updateMatchedPairs();
    mf.addTrack (seq);
    file.deleteFile();
    juce::FileOutputStream fos (file);
    if (! fos.openedOk()) return false;
    return mf.writeTo (fos);
}

juce::File KinoProcessor::createTempMidiFile() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory);
    auto f = dir.getChildFile ("Kino_Pattern.mid");
    exportMidiFile (f);
    return f;
}

void KinoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto stepsTree = juce::ValueTree ("Steps");
    {
        const juce::SpinLock::ScopedLockType sl (stepLock);
        for (int i = 0; i < maxSteps; ++i)
        {
            juce::ValueTree st ("Step");
            const auto& s = steps[(size_t) i];
            st.setProperty ("a", s.active, nullptr);
            st.setProperty ("d", s.degree, nullptr);
            st.setProperty ("v", s.velocity, nullptr);
            st.setProperty ("g", s.gate, nullptr);
            st.setProperty ("r", s.ratchet, nullptr);
            st.setProperty ("s", s.slide, nullptr);
            st.setProperty ("ac", s.accent, nullptr);
            st.setProperty ("p", s.prob, nullptr);
            st.setProperty ("o", s.octJump, nullptr);
            stepsTree.appendChild (st, nullptr);
        }
    }
    state.appendChild (stepsTree, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void KinoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (tree.isValid())
        {
            auto stepsTree = tree.getChildWithName ("Steps");
            if (stepsTree.isValid())
            {
                const juce::SpinLock::ScopedLockType sl (stepLock);
                for (int i = 0; i < std::min (maxSteps, stepsTree.getNumChildren()); ++i)
                {
                    auto st = stepsTree.getChild (i);
                    auto& s = steps[(size_t) i];
                    s.active   = (bool) st.getProperty ("a", false);
                    s.degree   = (int) st.getProperty ("d", 0);
                    s.velocity = (int) st.getProperty ("v", 100);
                    s.gate     = (float) st.getProperty ("g", 0.65f);
                    s.ratchet  = (int) st.getProperty ("r", 1);
                    s.slide    = (bool) st.getProperty ("s", false);
                    s.accent   = (bool) st.getProperty ("ac", false);
                    s.prob     = (int) st.getProperty ("p", 100);
                    s.octJump  = (int) st.getProperty ("o", 0);
                }
                tree.removeChild (stepsTree, nullptr);
            }
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* KinoProcessor::createEditor() { return new KinoEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KinoProcessor(); }
