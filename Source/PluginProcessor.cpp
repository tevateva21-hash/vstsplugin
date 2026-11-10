#include <climits>
#include <cstring>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace Eclipse;
using APVTS = juce::AudioProcessorValueTreeState;

static const double rateLen[4] = { 0.5, 0.25, 1.0 / 6.0, 0.125 };
static const double delayBeats[8] = { 0.25, 1.0 / 3.0, 0.5, 0.75, 2.0 / 3.0, 1.0, 1.5, 2.0 };

APVTS::ParameterLayout EclipseProcessor::createLayout()
{
    using F = juce::AudioParameterFloat; using R = juce::NormalisableRange<float>;
    APVTS::ParameterLayout l;
    auto pid = [] (const juce::String& s) { return juce::ParameterID { s, 1 }; };
    auto flt = [&] (const juce::String& id, const juce::String& n, float lo, float hi, float def, float skew = 1.0f)
    { l.add (std::make_unique<F> (pid (id), n, R (lo, hi, 0.0f, skew), def)); };

    l.add (std::make_unique<juce::AudioParameterFloat> (pid ("bpm"), "BPM interne", R (60.f, 200.f, 0.1f), 140.f));
    l.add (std::make_unique<juce::AudioParameterBool> (pid ("run"), "Lecture interne", false));
    flt ("swing", "Swing", 0.f, 1.f, 0.f);
    flt ("master", "Master", 0.f, 1.5f, 0.9f);
    juce::StringArray trk; for (int i = 1; i <= 8; ++i) trk.add ("Piste " + juce::String (i));
    l.add (std::make_unique<juce::AudioParameterChoice> (pid ("keyTrack"), "Clavier MIDI vers", trk, 0));

    juce::StringArray names; for (int i = 0; i < numSamples; ++i) names.add (sampleNames()[i]);
    names.add ("[ Mon sample ]");
    l.add (std::make_unique<juce::AudioParameterInt> (pid ("lane_steps"), "FX Lane pas", 2, maxSteps, 16));
    const int defSample[8] = { 0, 3, 6, 7, 10, 13, 16, 22 };
    for (int t = 0; t < numTracks; ++t)
    {
        auto p = "t" + juce::String (t) + "_"; auto n = "Piste " + juce::String (t + 1) + " ";
        l.add (std::make_unique<juce::AudioParameterChoice> (pid (p + "sample"), n + "sample", names, defSample[t]));
        flt (p + "vol", n + "vol", 0.f, 1.5f, 0.85f);
        flt (p + "pan", n + "pan", -1.f, 1.f, 0.f);
        flt (p + "pitch", n + "pitch", -24.f, 24.f, 0.f);
        flt (p + "len", n + "longueur", 0.05f, 1.f, 1.f);
        flt (p + "dly", n + "envoi delay", 0.f, 1.f, 0.f);
        flt (p + "rev", n + "envoi reverb", 0.f, 1.f, 0.f);
        l.add (std::make_unique<juce::AudioParameterBool> (pid (p + "mute"), n + "mute", false));
        l.add (std::make_unique<juce::AudioParameterChoice> (pid (p + "rate"), n + "grille", juce::StringArray { "1/8", "1/16", "1/16T", "1/32" }, 1));
        l.add (std::make_unique<juce::AudioParameterInt> (pid (p + "steps"), n + "pas", 2, maxSteps, 16));
    }

    l.add (std::make_unique<juce::AudioParameterChoice> (pid ("dlyTime"), "Delay temps",
        juce::StringArray { "1/16", "1/8T", "1/8", "1/8.", "1/4T", "1/4", "1/4.", "1/2" }, 3));
    flt ("dlyFb", "Feedback", 0.f, 0.95f, 0.55f);
    flt ("dlyTone", "Tone", 300.f, 12000.f, 4500.f, 0.4f);
    flt ("dlyWob", "Wobble", 0.f, 1.f, 0.35f);
    flt ("dlyWobRate", "Vitesse wobble", 0.05f, 6.f, 0.4f, 0.5f);
    flt ("dlySpread", "Ping-pong", 0.f, 1.f, 0.7f);
    flt ("dlySmear", "Smear", 0.f, 1.f, 0.4f);
    flt ("dlyDrive", "Drive", 0.f, 1.f, 0.25f);
    flt ("dlyMix", "Retour delay", 0.f, 1.f, 0.6f);
    flt ("rvSize", "Taille reverb", 0.f, 1.f, 0.8f);
    flt ("rvDamp", "Damping", 0.f, 1.f, 0.5f);
    flt ("rvMix", "Retour reverb", 0.f, 1.f, 0.5f);
    l.add (std::make_unique<juce::AudioParameterChoice> (pid ("fType"), "Filtre type", juce::StringArray { "Passe-bas", "Passe-haut", "Passe-bande" }, 0));
    flt ("fCut", "Cutoff", 20.f, 20000.f, 20000.f, 0.25f);
    flt ("fRes", "Résonance", 0.f, 1.f, 0.2f);
    flt ("fLfo", "LFO profondeur", 0.f, 1.f, 0.f);
    flt ("fRate", "LFO vitesse", 0.02f, 8.f, 0.25f, 0.4f);
    flt ("phRate", "Phaser vitesse", 0.05f, 5.f, 0.3f, 0.5f);
    flt ("phDepth", "Phaser profondeur", 0.f, 1.f, 0.7f);
    flt ("phMix", "Phaser mix", 0.f, 1.f, 0.f);
    return l;
}

EclipseProcessor::EclipseProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    auto g = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    for (int t = 0; t < numTracks; ++t)
    {
        auto p = "t" + juce::String (t) + "_";
        tp[t] = { g (p + "sample"), g (p + "vol"), g (p + "pan"), g (p + "pitch"), g (p + "len"),
                  g (p + "dly"), g (p + "rev"), g (p + "mute"), g (p + "rate"), g (p + "steps") };
        playStep[t] = -1; lastKey[t] = LLONG_MIN;
        for (int s = 0; s < maxSteps; ++s) { stepVel[t][s] = 0; stepFx[t][s] = 0; }
    }
    pLaneSteps = g ("lane_steps");
    for (auto& f : laneFx) f = 0;
    for (auto& u : userData) u = nullptr;
    formats.registerBasicFormats();
    pBpm = g ("bpm"); pRun = g ("run"); pSwing = g ("swing"); pMaster = g ("master"); pKeyTrack = g ("keyTrack");
    pDlyTime = g ("dlyTime"); pDlyFb = g ("dlyFb"); pDlyTone = g ("dlyTone"); pDlyWob = g ("dlyWob"); pDlyWobRate = g ("dlyWobRate");
    pDlySpread = g ("dlySpread"); pDlySmear = g ("dlySmear"); pDlyDrive = g ("dlyDrive"); pDlyMix = g ("dlyMix");
    pRvSize = g ("rvSize"); pRvDamp = g ("rvDamp"); pRvMix = g ("rvMix");
    pFType = g ("fType"); pFCut = g ("fCut"); pFRes = g ("fRes"); pFLfo = g ("fLfo"); pFRate = g ("fRate");
    pPhRate = g ("phRate"); pPhDepth = g ("phDepth"); pPhMix = g ("phMix");
    loadPreset (0);
}

void EclipseProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    sr = sampleRate;
    bank.clear();
    for (int i = 0; i < numSamples; ++i) bank.push_back (generateSample (i, sr));
    for (int t = 0; t < numTracks; ++t) if (userPath[t].isNotEmpty()) loadUserSample (t, juce::File (userPath[t]), false);
    ringL.assign ((size_t) (sr * 2.0), 0.f); ringR.assign ((size_t) (sr * 2.0), 0.f);
    laneFxBuf.assign ((size_t) blockSize, 0); laneProgBuf.assign ((size_t) blockSize, 0.f);
    ringPos = 0; prevLaneFx = 0; laneMix = 0; lag = 0;
    for (auto& tr : voices) for (auto& v : tr) v = Voice();
    for (auto& k : lastKey) k = LLONG_MIN;
    delay.prepare (sr);
    reverb.reset();
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };
    phaser.prepare (spec); phaser.setCentreFrequency (900.f); phaser.setFeedback (0.5f);
    filter.prepare (spec); filter.reset();
    dryBuf.setSize (2, blockSize); rvBuf.setSize (2, blockSize);
    internalPpq = 0; wasPlaying = false;
}

void EclipseProcessor::trigger (int t, float vel, int extraSemis, int fx)
{
    if (*tp[t].mute > 0.5f) return;
    const int si = juce::jlimit (0, numSamples, (int) *tp[t].sample);
    const std::vector<float>* d = nullptr;
    if (si == numSamples) d = userData[t].load();
    else if (si < (int) bank.size()) d = &bank[(size_t) si];
    if (d == nullptr || d->empty()) return;
    Voice* best = &voices[t][0];
    for (auto& v : voices[t]) { if (! v.active) { best = &v; break; } if (v.age > best->age) best = &v; }
    Voice nv;
    nv.data = d;
    const double base = std::pow (2.0, (*tp[t].pitch + extraSemis) / 12.0);
    nv.inc = base;
    nv.endPos = std::max (64.0, (double) d->size() * *tp[t].len * (fx == FxChop ? 0.15 : 1.0));
    nv.endPos = std::min (nv.endPos, (double) d->size());
    nv.rev = (fx == FxReverse);
    if (nv.rev) { nv.pos = nv.endPos - 1; nv.inc = -nv.inc; }
    nv.gain = vel; nv.sendBoost = (fx == FxThrow) ? 1.0f : 0.0f; nv.active = true;
    switch (fx)
    {
        case FxTapeStop:  nv.incSlope = -base / (0.45 * sr); break;
        case FxPitchRise: nv.incSlope = base * 2.5 / (0.5 * sr); break;
        case FxCrush:     nv.crush = true; break;
        case FxRing:      nv.ringInc = juce::MathConstants<double>::twoPi * 140.0 / sr; break;
        case FxLpClose:   nv.lpOn = true; nv.cutoff = 14000.f; nv.cutMul = (float) std::pow (150.0 / 14000.0, 1.0 / (0.35 * sr)); break;
        case FxLpOpen:    nv.lpOn = true; nv.cutoff = 150.f;   nv.cutMul = (float) std::pow (14000.0 / 150.0, 1.0 / (0.35 * sr)); break;
        case FxDrive:     nv.drive = true; break;
        case FxStutter:   nv.loops = 3; nv.loopEnd = std::max (256.0, nv.endPos * 0.125); break;
        default: break;
    }
    *best = nv;
}

void EclipseProcessor::stepTrack (int t, double ppq, float swing)
{
    const double stepLen = rateLen[juce::jlimit (0, 3, (int) *tp[t].rate)];
    const double pairPos = ppq / stepLen / 2.0;
    const double pi = std::floor (pairPos), f = pairPos - pi, b = 0.5 + 0.25 * swing;
    const int k = f < b ? 0 : 1;
    const double sub = k == 0 ? f / b : (f - b) / (1.0 - b);
    const long long idx = (long long) pi * 2 + k;
    const int steps = juce::jlimit (2, maxSteps, (int) *tp[t].steps);
    const int s = (int) (((idx % steps) + steps) % steps);
    playStep[t] = s;
    const int vel = stepVel[t][s], fx = stepFx[t][s];
    const int nr = fx == FxRat2 ? 2 : fx == FxRat3 ? 3 : fx == FxRat4 ? 4 : 1;
    const int subIdx = std::min ((int) (sub * nr), nr - 1);
    const long long key = idx * 8 + subIdx;
    if (key == lastKey[t]) return;
    const bool first = lastKey[t] == LLONG_MIN;
    lastKey[t] = key;
    if (first && (sub * nr - subIdx) > 0.05) return;   // démarrage au milieu d'un pas
    if (vel == 0) return;
    if (fx == FxProb50 && rng.nextBool()) return;
    int semis = fx == FxOctUp ? 12 : fx == FxOctDown ? -12 : 0;
    if (fx == FxRandom) { static const int sc[8] = { -12, -7, -5, 0, 3, 5, 7, 12 }; semis = sc[rng.nextInt (8)]; }
    const float v = (vel / 127.0f) * (1.0f - 0.12f * (float) subIdx);
    trigger (t, v, semis, fx);
}

void EclipseProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (! m.isNoteOn()) return;
    const int n = m.getNoteNumber(); const float v = m.getFloatVelocity();
    if (n >= 36 && n < 36 + numTracks) trigger (n - 36, v, 0, FxNone);
    else trigger (juce::jlimit (0, numTracks - 1, (int) *pKeyTrack), v, n - 60, FxNone);
}

void EclipseProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (bank.empty()) return;
    if (dryBuf.getNumSamples() < n) { dryBuf.setSize (2, n, false, false, true); rvBuf.setSize (2, n, false, false, true); }
    dryBuf.clear(); rvBuf.clear();

    // --- transport ---
    bool hostPlaying = false, hasPpq = false, hasBpm = false; double hostPpq = 0, hostBpm = 120;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { hostPpq = *p; hasPpq = true; }
            if (auto b = pos->getBpm()) { hostBpm = *b; hasBpm = true; }
        }
    const bool internalRun = *pRun > 0.5f;
    const bool useHost = hostPlaying && hasPpq;
    const bool playing = useHost || internalRun;
    const double bpm = (hasBpm && ! internalRun) ? hostBpm : (double) *pBpm;
    double ppq = useHost ? hostPpq : internalPpq;
    const double pps = bpm / 60.0 / sr;
    if (playing && ! wasPlaying) for (auto& k : lastKey) k = LLONG_MIN;
    if (! playing) { for (auto& s : playStep) s = -1; internalPpq = 0; }
    wasPlaying = playing;

    // --- paramètres de bloc ---
    const float swing = *pSwing;
    bool anySolo = false; (void) anySolo;
    for (int t = 0; t < numTracks; ++t)
    {
        const float pan = *tp[t].pan, ang = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        trackVol[t] = *tp[t].vol; trackGainL[t] = std::cos (ang); trackGainR[t] = std::sin (ang);
        trackDly[t] = *tp[t].dly; trackRev[t] = *tp[t].rev;
    }
    PsyDelay::Params dp;
    dp.feedback = *pDlyFb; dp.toneHz = *pDlyTone; dp.wobble = *pDlyWob; dp.wobbleRate = *pDlyWobRate;
    dp.spread = *pDlySpread; dp.smear = *pDlySmear; dp.drive = *pDlyDrive;
    const double targetDelay = delayBeats[juce::jlimit (0, 7, (int) *pDlyTime)] * 60.0 / bpm * sr;
    const float dlyMix = *pDlyMix;
    const float master = *pMaster;

    auto* oL = buffer.getWritePointer (0); auto* oR = buffer.getWritePointer (1);
    auto* dL = dryBuf.getWritePointer (0); auto* dR = dryBuf.getWritePointer (1);
    auto* rL = rvBuf.getWritePointer (0);  auto* rR = rvBuf.getWritePointer (1);

    if ((int) laneFxBuf.size() < n) { laneFxBuf.assign ((size_t) n, 0); laneProgBuf.assign ((size_t) n, 0.f); }
    const int laneSteps = juce::jlimit (2, maxSteps, (int) *pLaneSteps);
    auto midiIt = midi.begin();
    for (int i = 0; i < n; ++i)
    {
        while (midiIt != midi.end() && (*midiIt).samplePosition <= i) { handleMidi ((*midiIt).getMessage()); ++midiIt; }
        if (playing) for (int t = 0; t < numTracks; ++t) stepTrack (t, ppq + i * pps, swing);

        int lfx = 0; float lpr = 0;
        if (playing)
        {
            const double lp = (ppq + i * pps) / 0.25; const double li = std::floor (lp);
            const int ls = (int) (((long long) li % laneSteps + laneSteps) % laneSteps);
            lfx = laneFx[ls]; lpr = (float) (lp - li); lanePlay = ls;
        }
        else lanePlay = -1;
        laneFxBuf[(size_t) i] = (uint8_t) lfx; laneProgBuf[(size_t) i] = lpr;
        float dlSumL = 0, dlSumR = 0;
        for (int t = 0; t < numTracks; ++t)
            for (auto& v : voices[t])
            {
                if (! v.active) continue;
                auto& d = *v.data; const int sz = (int) d.size();
                const int i0 = std::max (0, (int) v.pos); const int i1 = std::min (i0 + 1, sz - 1);
                const float fr = (float) (v.pos - i0);
                float s = d[(size_t) i0] + (d[(size_t) i1] - d[(size_t) i0]) * fr;
                if (v.crush) { if (--v.crushCnt <= 0) { v.held = std::round (s * 8.0f) / 8.0f; v.crushCnt = 6; } s = v.held; }
                if (v.ringInc > 0) { v.ringPh += v.ringInc; s *= (float) std::sin (v.ringPh); }
                if (v.lpOn) { s = v.svf.process (s, v.cutoff, 0.6f, sr, 0); v.cutoff = juce::jlimit (80.0f, 16000.0f, v.cutoff * v.cutMul); }
                if (v.drive) s = std::tanh (s * 5.0f) * 0.6f;
                const double lim = v.loops > 0 ? v.loopEnd : v.endPos;
                const double rem = v.rev ? v.pos : lim - v.pos;
                const double fadeLen = 128.0 * std::abs (v.inc);
                float env = rem < fadeLen ? (float) (rem / fadeLen) : 1.0f;
                if (v.incSlope < 0) env *= (float) juce::jmin (1.0, v.inc / 0.15);
                if (v.age < 64) env *= (float) v.age / 64.0f;
                s *= env * v.gain * trackVol[t];
                const float sL = s * trackGainL[t], sR = s * trackGainR[t];
                dL[i] += sL; dR[i] += sR;
                const float ds = std::min (1.0f, trackDly[t] + v.sendBoost);
                dlSumL += sL * ds; dlSumR += sR * ds;
                rL[i] += sL * trackRev[t]; rR[i] += sR * trackRev[t];
                v.pos += v.inc; ++v.age;
                if (v.incSlope != 0) { v.inc += v.incSlope; if (v.incSlope < 0 && v.inc < 0.03) v.active = false; }
                if (v.loops > 0 && v.pos >= v.loopEnd) { v.pos -= v.loopEnd; if (--v.loops == 0) v.endPos = v.loopEnd; v.age = 0; }
                else if (v.pos >= v.endPos || v.pos < 0) v.active = false;
            }

        if (lfx == LaneThrow) { dlSumL += dL[i]; dlSumR += dR[i]; }
        if (lfx == LaneSwell) { rL[i] += dL[i] * 0.8f; rR[i] += dR[i] * 0.8f; }
        auto dpl = dp;
        if (lfx == LaneFreeze) { dlSumL = dlSumR = 0; dpl.feedback = 0.997f; }
        delayTime += (targetDelay - delayTime) * 0.0004;
        dpl.timeSamples = (float) delayTime;
        float yl, yr; delay.process (dlSumL, dlSumR, yl, yr, dpl);
        oL[i] = dL[i] + yl * dlyMix; oR[i] = dR[i] + yr * dlyMix;
    }
    if (! useHost && internalRun) internalPpq += n * pps;

    // reverb (retour)
    reverb.setParameters ({ *pRvSize, *pRvDamp, 1.0f, 0.0f, 0.0f, 0.0f });
    reverb.processStereo (rL, rR, n);
    const float rvMix = *pRvMix;
    for (int i = 0; i < n; ++i) { oL[i] += rL[i] * rvMix; oR[i] += rR[i] * rvMix; }

    // filtre maître avec LFO
    filter.setType (*pFType < 0.5f ? juce::dsp::StateVariableTPTFilterType::lowpass
                  : *pFType < 1.5f ? juce::dsp::StateVariableTPTFilterType::highpass
                                   : juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.setResonance (0.7f + *pFRes * 8.0f);
    const float baseCut = *pFCut, lfoDepth = *pFLfo; const double lfoInc = *pFRate / sr;
    const bool filterActive = baseCut < 19000.0f || lfoDepth > 0.001f || *pFType > 0.5f;
    if (filterActive)
        for (int s0 = 0; s0 < n; s0 += 32)
        {
            filterPhase += lfoInc * 32; filterPhase -= std::floor (filterPhase);
            float fc = baseCut * std::pow (2.0f, lfoDepth * 3.0f * (float) std::sin (juce::MathConstants<double>::twoPi * filterPhase));
            filter.setCutoffFrequency (juce::jlimit (20.0f, 20000.0f, fc));
            for (int i = s0; i < std::min (n, s0 + 32); ++i) { oL[i] = filter.processSample (0, oL[i]); oR[i] = filter.processSample (1, oR[i]); }
        }

    // phaser
    if (*pPhMix > 0.001f)
    {
        phaser.setRate (*pPhRate); phaser.setDepth (*pPhDepth); phaser.setMix (*pPhMix);
        juce::dsp::AudioBlock<float> blk (buffer);
        phaser.process (juce::dsp::ProcessContextReplacing<float> (blk));
    }
    // --- FX lane : effets maitre places sur la grille ---
    const int ringSize = (int) ringL.size();
    const double beatSamples = 60.0 / bpm * sr;
    for (int i = 0; i < n; ++i)
    {
        const float l = oL[i], r = oR[i];
        ringL[(size_t) ringPos] = l; ringR[(size_t) ringPos] = r;
        const int fx = laneFxBuf[(size_t) i]; const float pr = laneProgBuf[(size_t) i];
        if (fx != prevLaneFx)
        {
            if (fx == LaneRep16 || fx == LaneRep32)
            {
                repeatLen = juce::jlimit (64, ringSize - 2, (int) (beatSamples * (fx == LaneRep16 ? 0.25 : 0.125)));
                repeatStart = ((ringPos - repeatLen) % ringSize + ringSize) % ringSize; repeatPos = 0;
            }
            if (fx == LaneTapeStop) lag = 0;
            for (auto& f : lanF) f = Eclipse::SVF();
            prevLaneFx = fx;
        }
        float wl = l, wr = r;
        switch (fx)
        {
            case LaneLpUp:   { float fc = 150.f * std::pow (120.f, pr);   wl = lanF[0].process (l, fc, 0.6f, sr, 0); wr = lanF[1].process (r, fc, 0.6f, sr, 0); break; }
            case LaneLpDown: { float fc = 18000.f * std::pow (1.f / 120.f, pr); wl = lanF[0].process (l, fc, 0.6f, sr, 0); wr = lanF[1].process (r, fc, 0.6f, sr, 0); break; }
            case LaneHpUp:   { float fc = 40.f * std::pow (150.f, pr);    wl = lanF[0].process (l, fc, 0.9f, sr, 2); wr = lanF[1].process (r, fc, 0.9f, sr, 2); break; }
            case LaneRep16: case LaneRep32:
            {
                const int rp = (repeatStart + repeatPos) % ringSize; wl = ringL[(size_t) rp]; wr = ringR[(size_t) rp];
                if (++repeatPos >= repeatLen) repeatPos = 0; break;
            }
            case LaneTapeStop:
            {
                lag = std::min (lag + juce::jlimit (0.0f, 1.0f, pr * 1.1f), (double) ringSize - 4.0);
                double rp = (double) ringPos - lag; if (rp < 0) rp += ringSize;
                const int i0 = (int) rp, i1 = (i0 + 1) % ringSize; const float fr = (float) (rp - i0);
                wl = ringL[(size_t) i0] + (ringL[(size_t) i1] - ringL[(size_t) i0]) * fr;
                wr = ringR[(size_t) i0] + (ringR[(size_t) i1] - ringR[(size_t) i0]) * fr;
                const float amp = 1.0f - pr * 0.6f; wl *= amp; wr *= amp; break;
            }
            case LaneCrush:  { if (--crushCnt <= 0) { heldL = std::round (l * 8.f) / 8.f; heldR = std::round (r * 8.f) / 8.f; crushCnt = 6; } wl = heldL; wr = heldR; break; }
            case LaneGate:   { const float g = std::fmod (pr * 4.0f, 1.0f) < 0.5f ? 1.0f : 0.04f; wl = l * g; wr = r * g; break; }
            case LaneRing:   { ringPh += 220.0 / sr; ringPh -= std::floor (ringPh); const float m = (float) std::sin (juce::MathConstants<double>::twoPi * ringPh); wl = l * m; wr = r * m; break; }
            case LaneDrive:  { wl = std::tanh (l * 6.f) * 0.7f; wr = std::tanh (r * 6.f) * 0.7f; break; }
            case LaneDrop:   { wl = wr = 0; break; }
            default: lag = 0; break;
        }
        const bool active = fx != LaneNone && fx != LaneThrow && fx != LaneFreeze && fx != LaneSwell;
        laneMix += ((active ? 1.0f : 0.0f) - laneMix) * 0.01f;
        oL[i] = l + (wl - l) * laneMix; oR[i] = r + (wr - r) * laneMix;
        if (++ringPos >= ringSize) ringPos = 0;
    }
    for (int i = 0; i < n; ++i) { oL[i] = std::tanh (oL[i] * master); oR[i] = std::tanh (oR[i] * master); }
}

// ------------------------------------------------------------------ grille
void EclipseProcessor::clearTrack (int t) { for (int s = 0; s < maxSteps; ++s) setStep (t, s, 0, 0); }

void EclipseProcessor::euclid (int t, int hits, int rot)
{
    const int steps = getSteps (t);
    clearTrack (t);
    for (int i = 0; i < steps; ++i)
        if ((i * hits) % steps < hits) setStep (t, (i + rot + steps) % steps, i == 0 ? 120 : 100, 0);
}

void EclipseProcessor::randomizeTrack (int t, float density)
{
    const int steps = getSteps (t); clearTrack (t);
    for (int s = 0; s < steps; ++s)
        if (rng.nextFloat() < density)
        {
            int fx = rng.nextFloat() < 0.2f ? 1 + rng.nextInt (FxCount - 1) : 0;
            setStep (t, s, 50 + rng.nextInt (78), fx);
        }
}

void EclipseProcessor::shiftTrack (int t, int a)
{
    const int steps = getSteps (t); uint8_t v[maxSteps], f[maxSteps];
    for (int s = 0; s < steps; ++s) { v[(s + a + steps) % steps] = stepVel[t][s]; f[(s + a + steps) % steps] = stepFx[t][s]; }
    for (int s = 0; s < steps; ++s) setStep (t, s, v[s], f[s]);
}

void EclipseProcessor::reverseTrack (int t)
{
    const int steps = getSteps (t);
    for (int s = 0; s < steps / 2; ++s)
    {
        int a = stepVel[t][s], af = stepFx[t][s], o = steps - 1 - s;
        setStep (t, s, stepVel[t][o], stepFx[t][o]); setStep (t, o, a, af);
    }
}

void EclipseProcessor::randomizeLane (float density)
{
    clearLane();
    for (int s = 0; s < getLaneSteps(); ++s)
        if (rng.nextFloat() < density) setLane (s, 1 + rng.nextInt (LaneCount - 1));
}

bool EclipseProcessor::loadUserSample (int t, const juce::File& file, bool selectOnLoad)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0) return false;
    const int len = (int) std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 20.0));
    juce::AudioBuffer<float> tmp ((int) std::max<unsigned> (1, reader->numChannels), len);
    reader->read (&tmp, 0, len, 0, true, true);
    const double ratio = reader->sampleRate / sr;
    const int outLen = std::max (16, (int) (len / ratio));
    auto out = std::make_unique<std::vector<float>> ((size_t) outLen, 0.f);
    float pk = 1e-6f;
    for (int i = 0; i < outLen; ++i)
    {
        const double pos = i * ratio; const int i0 = std::min ((int) pos, len - 1), i1 = std::min (i0 + 1, len - 1);
        const float fr = (float) (pos - i0); float m = 0;
        for (int c = 0; c < tmp.getNumChannels(); ++c) { auto* d = tmp.getReadPointer (c); m += d[i0] + (d[i1] - d[i0]) * fr; }
        m /= (float) tmp.getNumChannels(); (*out)[(size_t) i] = m; pk = std::max (pk, std::abs (m));
    }
    const float g = pk > 0.9f ? 0.9f / pk : 1.0f;
    for (auto& v : *out) v *= g;
    const size_t fade = std::min<size_t> (out->size(), 96);
    for (size_t i = 0; i < fade; ++i) (*out)[out->size() - 1 - i] *= (float) i / (float) fade;
    auto* raw = out.get();
    graveyard.push_back (std::move (out));
    userData[t] = raw;
    userPath[t] = file.getFullPathName();
    if (selectOnLoad)
        if (auto* prm = apvts.getParameter ("t" + juce::String (t) + "_sample"))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) numSamples));
    return true;
}

void EclipseProcessor::applyStepChar (int t, int s, char c)
{
    switch (c)
    {
        case 'x': setStep (t, s, 100, 0); break;
        case 'X': setStep (t, s, 127, 0); break;
        case 'o': setStep (t, s, 55, 0); break;
        case '2': setStep (t, s, 100, FxRat2); break;
        case '3': setStep (t, s, 100, FxRat3); break;
        case '4': setStep (t, s, 100, FxRat4); break;
        case 'v': setStep (t, s, 100, FxReverse); break;
        case 'u': setStep (t, s, 100, FxOctUp); break;
        case 'd': setStep (t, s, 100, FxOctDown); break;
        case 'c': setStep (t, s, 100, FxChop); break;
        case 't': setStep (t, s, 100, FxThrow); break;
        case '?': setStep (t, s, 100, FxRandom); break;
        case 'p': setStep (t, s, 100, FxProb50); break;
        default:  setStep (t, s, 0, 0); break;
    }
}

void EclipseProcessor::loadPreset (int index)
{
    int count; auto* presets = patternPresets (count);
    auto& p = presets[juce::jlimit (0, count - 1, index)];
    auto setParam = [this] (const juce::String& id, float actual)
    { if (auto* prm = apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (actual)); };
    setParam ("bpm", (float) p.bpm);
    for (int t = 0; t < numTracks; ++t)
    {
        const int len = (int) std::strlen (p.rows[t]);
        setParam ("t" + juce::String (t) + "_sample", (float) p.samples[t]);
        setParam ("t" + juce::String (t) + "_steps", (float) len);
        clearTrack (t);
        for (int s = 0; s < len && s < maxSteps; ++s) applyStepChar (t, s, p.rows[t][s]);
    }
    clearLane();
    const char* lanes = presetLanes (juce::jlimit (0, count - 1, index));
    setParam ("lane_steps", (float) std::strlen (lanes));
    for (int s = 0; lanes[s] != 0 && s < maxSteps; ++s)
        if (auto* q = std::strchr (laneChars(), lanes[s])) setLane (s, (int) (q - laneChars()));
    // quelques réglages de départ cohérents
    setParam ("t5_dly", 0.f); setParam ("t6_dly", 0.45f); setParam ("t7_dly", 0.6f); setParam ("t7_rev", 0.5f);
    setParam ("t3_rev", 0.2f); setParam ("t1_rev", 0.25f);
}

bool EclipseProcessor::exportMidi (const juce::File& file) const
{
    const int tpq = 960; juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / *pBpm)), 0);
    for (int t = 0; t < numTracks; ++t)
    {
        const double len = rateLen[juce::jlimit (0, 3, (int) *tp[t].rate)]; const int steps = getSteps (t);
        for (int i = 0; i < (int) (16.0 / len); ++i)
        {
            const int s = i % steps; const int v = stepVel[t][s]; if (v == 0) continue;
            const int fx = stepFx[t][s]; const int nr = fx == FxRat2 ? 2 : fx == FxRat3 ? 3 : fx == FxRat4 ? 4 : 1;
            for (int r = 0; r < nr; ++r)
            {
                const double start = (i + (double) r / nr) * len, dur = len / nr * 0.9;
                seq.addEvent (juce::MidiMessage::noteOn (1, 36 + t, (juce::uint8) v), start * tpq);
                seq.addEvent (juce::MidiMessage::noteOff (1, 36 + t), (start + dur) * tpq);
            }
        }
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (tpq); mf.addTrack (seq);
    file.deleteFile();
    juce::FileOutputStream os (file);
    return os.openedOk() && mf.writeTo (os);
}

// ------------------------------------------------------------------ état
void EclipseProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto xml = apvts.copyState().createXml();
    auto* st = xml->createNewChildElement ("STEPS");
    for (int t = 0; t < numTracks; ++t)
    {
        juce::String s;
        for (int i = 0; i < maxSteps; ++i) s << stepVel[t][i].load() << ":" << stepFx[t][i].load() << ",";
        st->setAttribute ("t" + juce::String (t), s);
        st->setAttribute ("u" + juce::String (t), userPath[t]);
    }
    juce::String ls; for (int i = 0; i < maxSteps; ++i) ls << laneFx[i].load() << ",";
    st->setAttribute ("lane", ls);
    copyXmlToBinary (*xml, dest);
}

void EclipseProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (auto* st = xml->getChildByName ("STEPS"))
        {
            for (int t = 0; t < numTracks; ++t)
            {
                auto parts = juce::StringArray::fromTokens (st->getStringAttribute ("t" + juce::String (t)), ",", "");
                for (int i = 0; i < maxSteps && i < parts.size(); ++i)
                {
                    auto kv = juce::StringArray::fromTokens (parts[i], ":", "");
                    if (kv.size() == 2) setStep (t, i, kv[0].getIntValue(), kv[1].getIntValue());
                }
            }
            auto lp = juce::StringArray::fromTokens (st->getStringAttribute ("lane"), ",", "");
            for (int i = 0; i < maxSteps && i < lp.size(); ++i) setLane (i, lp[i].getIntValue());
            for (int t = 0; t < numTracks; ++t)
            {
                auto path = st->getStringAttribute ("u" + juce::String (t));
                if (path.isNotEmpty() && juce::File (path).existsAsFile()) loadUserSample (t, juce::File (path), false);
            }
            xml->removeChildElement (st, true);
        }
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessorEditor* EclipseProcessor::createEditor() { return new EclipseEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new EclipseProcessor(); }
