#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "SampleBank.h"
#include "PsyDelay.h"
#include "Patterns.h"

namespace Eclipse
{
constexpr int numTracks = 8;
constexpr int maxSteps  = 32;
constexpr int voicesPerTrack = 6;

enum StepFx { FxNone = 0, FxRat2, FxRat3, FxRat4, FxReverse, FxOctUp, FxOctDown, FxChop, FxThrow, FxRandom, FxProb50,
               FxTapeStop, FxPitchRise, FxCrush, FxRing, FxLpClose, FxLpOpen, FxDrive, FxStutter, FxCount };
inline const char* fxName (int f)
{
    static const char* n[FxCount] = { "Normal", "Ratchet x2", "Ratchet x3", "Ratchet x4", "Reverse", "Octave +", "Octave -", "Chop", "Echo Throw", "Pitch Random", "Proba 50%",
        "Tape Stop", "Pitch Rise", "Bitcrush", "Ring Mod", "Filtre ferme", "Filtre ouvre", "Distorsion", "Stutter" };
    return n[juce::jlimit (0, FxCount - 1, f)];
}
inline const char* fxTag (int f)
{
    static const char* n[FxCount] = { "", "2", "3", "4", "R", "+", "-", "C", "T", "?", "%", "<", ">", "B", "M", "\\", "/", "D", "S" };
    return n[juce::jlimit (0, FxCount - 1, f)];
}

// Effets "maitre" places sur la piste FX LANE
enum LaneFx { LaneNone = 0, LaneThrow, LaneFreeze, LaneSwell, LaneLpUp, LaneLpDown, LaneHpUp, LaneRep16, LaneRep32,
              LaneTapeStop, LaneCrush, LaneGate, LaneRing, LaneDrive, LaneDrop, LaneCount };
inline const char* laneName (int f)
{
    static const char* n[LaneCount] = { "Aucun", "Delay Throw", "Delay Freeze", "Reverb Swell", "Filtre LP monte", "Filtre LP descend", "Filtre HP monte",
        "Beat Repeat 1/16", "Beat Repeat 1/32", "Tape Stop master", "Bitcrush master", "Trance Gate", "Ring Mod", "Distorsion", "Drop (silence)" };
    return n[juce::jlimit (0, LaneCount - 1, f)];
}
inline const char* laneChars() { return ".TFW/\\HRr<BGMDX"; }
inline const char* laneTag (int f)
{
    static const char* n[LaneCount] = { "", "T", "F", "W", "/", "\\", "H", "R", "r", "<", "B", "G", "M", "D", "X" };
    return n[juce::jlimit (0, LaneCount - 1, f)];
}
}

class EclipseProcessor : public juce::AudioProcessor
{
public:
    EclipseProcessor();
    ~EclipseProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& l) const override { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Eclipse Psy Sampler"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // --- grille ---
    void setStep (int track, int step, int vel, int fx) { stepVel[track][step] = (uint8_t) vel; stepFx[track][step] = (uint8_t) fx; }
    int  getVel (int t, int s) const { return stepVel[t][s]; }
    int  getFx  (int t, int s) const { return stepFx[t][s]; }
    int  getSteps (int t) const { return (int) *tp[t].steps; }
    void clearTrack (int t);
    void clearAll() { for (int t = 0; t < Eclipse::numTracks; ++t) clearTrack (t); }
    void euclid (int t, int hits, int rotation);
    void randomizeTrack (int t, float density);
    void shiftTrack (int t, int amount);
    void reverseTrack (int t);
    void loadPreset (int index);
    // --- FX lane (effets maitre sur la grille) ---
    void setLane (int step, int fx) { laneFx[step] = (uint8_t) fx; }
    int  getLane (int step) const { return laneFx[step]; }
    int  getLaneSteps() const { return (int) *pLaneSteps; }
    void clearLane() { for (auto& f : laneFx) f = 0; }
    void randomizeLane (float density);
    // --- samples utilisateur ---
    bool loadUserSample (int track, const juce::File& file, bool selectOnLoad = true);
    juce::String getUserName (int t) const { return userPath[t].isEmpty() ? juce::String() : juce::File (userPath[t]).getFileName(); }
    bool exportMidi (const juce::File& file) const;

    std::atomic<int> playStep[Eclipse::numTracks];
    std::atomic<int> lanePlay { -1 };
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void applyStepChar (int t, int s, char c);

    struct Voice
    {
        const std::vector<float>* data = nullptr;
        double pos = 0, inc = 1, endPos = 0;
        bool rev = false, active = false;
        float gain = 0, sendBoost = 0;
        int age = 0;
        // effets par pas
        double incSlope = 0, loopEnd = 0; int loops = 0;
        bool crush = false, drive = false, lpOn = false; int crushCnt = 0; float held = 0;
        double ringPh = 0, ringInc = 0; float cutoff = 0, cutMul = 1; Eclipse::SVF svf;
    };
    struct TrackParams { std::atomic<float> *sample, *vol, *pan, *pitch, *len, *dly, *rev, *mute, *rate, *steps; };

    void trigger (int t, float vel, int extraSemis, int fx);
    void stepTrack (int t, double ppq, float swing);
    void handleMidi (const juce::MidiMessage&);

    std::atomic<uint8_t> stepVel[Eclipse::numTracks][Eclipse::maxSteps];
    std::atomic<uint8_t> stepFx[Eclipse::numTracks][Eclipse::maxSteps];
    TrackParams tp[Eclipse::numTracks];
    std::atomic<float> *pBpm, *pRun, *pSwing, *pMaster, *pKeyTrack;
    std::atomic<float> *pDlyTime, *pDlyFb, *pDlyTone, *pDlyWob, *pDlyWobRate, *pDlySpread, *pDlySmear, *pDlyDrive, *pDlyMix;
    std::atomic<float> *pRvSize, *pRvDamp, *pRvMix;
    std::atomic<float> *pFType, *pFCut, *pFRes, *pFLfo, *pFRate;
    std::atomic<float> *pPhRate, *pPhDepth, *pPhMix;

    std::atomic<uint8_t> laneFx[Eclipse::maxSteps];
    std::atomic<float>* pLaneSteps;
    std::atomic<const std::vector<float>*> userData[Eclipse::numTracks];
    juce::String userPath[Eclipse::numTracks];
    std::vector<std::unique_ptr<std::vector<float>>> graveyard;   // garde les anciens samples valides pour le thread audio
    juce::AudioFormatManager formats;
    std::vector<float> ringL, ringR; std::vector<uint8_t> laneFxBuf; std::vector<float> laneProgBuf;
    float laneMix = 0, heldL = 0, heldR = 0; int prevLaneFx = 0, ringPos = 0, repeatPos = 0, repeatLen = 1000, crushCnt = 0;
    double lag = 0, ringPh = 0; int repeatStart = 0; Eclipse::SVF lanF[2];

    std::vector<std::vector<float>> bank;
    Voice voices[Eclipse::numTracks][Eclipse::voicesPerTrack];
    long long lastKey[Eclipse::numTracks];
    juce::Random rng;
    double sr = 44100.0, internalPpq = 0.0, delayTime = 10000.0, filterPhase = 0.0;
    bool wasPlaying = false;
    float trackVol[Eclipse::numTracks], trackGainL[Eclipse::numTracks], trackGainR[Eclipse::numTracks],
          trackDly[Eclipse::numTracks], trackRev[Eclipse::numTracks];

    Eclipse::PsyDelay delay;
    juce::Reverb reverb;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::AudioBuffer<float> dryBuf, rvBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EclipseProcessor)
};
