#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include "Gen.h"

namespace ut
{
inline const juce::StringArray keyNames { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
inline const juce::StringArray barNames { "1 bar", "2 bars", "4 bars", "8 bars", "16 bars" };
inline const int barVals[] { 1, 2, 4, 8, 16 };
inline const juce::StringArray soundNames { "Sub", "Rubber", "Reese", "Organ", "Acid", "Growl" };

struct Voice
{
    double sr = 44100;
    double phase = 0, phase2 = 0, phase3 = 0, subPhase = 0, lfo = 0, growl = 0;
    float pitchEnv = 0, perc = 0;
    float freq = 55, target = 55, glideCoef = 1;
    float amp = 0, fenv = 0;
    bool gate = false, arm = false;
    int note = -1, pendingNote = -1;
    float vel = 1, pendingVel = 0;
    float ic1 = 0, ic2 = 0, ic1R = 0, ic2R = 0; // SVF, left and right
    float dcX = 0, dcY = 0, dcXR = 0, dcYR = 0;
    void noteOn (int n, float v, bool slide, float glideMs);
    void noteOff (int n)
    {
        if (arm && n == pendingNote) { arm = false; pendingNote = -1; gate = false; return; }
        if (n == note) gate = false;
    }
    void allOff() { gate = false; arm = false; pendingNote = -1; note = -1; }
    void render (int sound, float cutoffOffset, float drive, float& left, float& right);
    void startNow (int n, float v);
};

struct Kick
{
    double sr = 44100, t = 1e9, ph = 0;
    float tune = 49;
    void trig (float hz) { t = 0; ph = 0; tune = hz; }
    float render();
};
}

class UndertowProcessor : public juce::AudioProcessor, private juce::Timer, private juce::AudioProcessorValueTreeState::Listener
{
public:
    UndertowProcessor();
    ~UndertowProcessor() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Undertow"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // generator control (message thread)
    void generateNew();   // new seeds (respects locks)
    void mutate();
    void undo();
    void redo();
    bool canUndo() const { return ! history.empty(); }
    bool canRedo() const { return ! future.empty(); }
    void regenerate();
    void pump() { timerCallback(); } // run pending regeneration now (tests, offline tools)
    // hand edits from the piano roll: begin pushes one undo step, apply updates the pattern live
    void beginEdit();
    void applyEdit (std::vector<ut::Note> notes);
    bool isEdited() const { return edited; }
    void getBarRoots (int* out) const { ut::barRoots (currentParams(), out); }
    // factory presets
    struct Preset { const char* name; int style, scale, chords; float density, movement, variation, gate, swing, glide, ghosts, accent; int sound; float tone, drive; };
    static const std::vector<Preset>& presets();
    void applyPreset (int index);
    int currentPreset = -1;
    juce::String activeUserPreset;
    bool presetModified = false;
    std::vector<ut::Note> getPattern() const { const juce::SpinLock::ScopedLockType l (patLock); return pattern; }
    ut::Params currentParams() const;
    int rootMidi() const;
    int scale() const { return (int) p ("scale"); }
    int bars() const { return ut::barVals[(int) p ("bars")]; }
    float swing() const { return p ("swing"); }
    bool writeMidi (const juce::File& f, int takes = 1) const;
    int printTakes() const; // how many takes DRAG/EXPORT writes (EVOLVE on: several takes back to back)
    int playMode() const { return (int) p ("playMode"); }
    // user presets live in the app-data folder, separate from the factory list
    juce::File userPresetFile() const;
    struct UserPreset { juce::String name; juce::String xml; };
    std::vector<UserPreset> userPresets;
    void refreshUserPresets();
    void saveUserPreset (const juce::String& name);
    void deleteUserPreset (int index);
    void renameUserPreset (int index, const juce::String& name);
    void applyUserPreset (int index);
    // idea slots: save up to 8 basslines (pattern + settings) and flip between them
    static constexpr int NumSlots = 8;
    bool slotUsed (int i) const { return slots[(size_t) i].used; }
    void storeSlot (int i); void recallSlot (int i); void clearSlot (int i);
    int currentSlot = -1;
    juce::String suggestedFileName() const;

    // transport
    std::atomic<bool> internalPlay { false };
    std::atomic<float> uiBeat { -1 }, bpm { 128 };
    std::atomic<int> uiNoteIdx { -1 }, patternVersion { 0 };
    std::atomic<float> uiLevel { 0 };
    std::array<float, 1024> scope {}; std::atomic<int> scopeW { 0 };

    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

private:
    void timerCallback() override;
    void parameterChanged (const juce::String& id, float) override;
    std::atomic<bool> dirty { true }, renderDirty { false };
    std::atomic<bool> fromProject { false }; // loaded notes are the pattern until a generator control changes

    struct Seeds { uint32_t r = 11, p = 7, m = 0, take = 0; int muts = 0; };
    Seeds seeds;
    struct Snapshot { Seeds seeds; std::vector<ut::Note> pat; bool edited = false; };
    std::vector<Snapshot> history, future;
    bool edited = false, applyingPreset = false;
    Snapshot snap() const;
    void restore (const Snapshot&);
    void pushHistory();
    juce::Random rnd;

    mutable juce::SpinLock patLock;
    std::vector<ut::Note> pattern;

    struct Ev { double beat; bool on; int note; int vel; bool slide; int idx; };
    std::vector<Ev> events; // rebuilt with the pattern, read under patLock
    std::atomic<double> loopBeats { 8 };
    void buildEvents (const std::vector<ut::Note>& pat, std::vector<Ev>& out, double& loop) const;
    void rebuildEvents();
    uint32_t liveTake = 0;
    std::vector<ut::Note> stagedPat;
    std::vector<Ev> stagedEv;
    uint32_t stagedTake = 0xffffffffu;
    bool stagedReady = false;
    std::atomic<uint32_t> stageRequest { 0 };
    std::atomic<bool> cycleArmed { false };
    std::atomic<long long> cycleOrigin { 0 };

    ut::Voice voice; ut::Kick kick;
    double sr = 44100, internalPpq = 0; bool wasPlaying = false;
    int sounding = -1;
    uint64_t soundingLo = 0, soundingHi = 0; // MIDI pitches actually held, so a pattern swap can release them
    float duck = 1, duckGoal = 1;
    int duckHold = 0;
    int midiLatch = -1; // root from MIDI, not written back to the host key parameter
    bool applyingFeel = false;
    std::atomic<int> pendingSwingStyle { -1 };
    std::atomic<bool> swingTouched { false };
    std::array<uint8_t, 128> heldNotes {};
    double hostOffset = 0, lastHostEnd = -1; long long lastCycle = -1;
    std::atomic<bool> wrapped { false };
    struct Slot { bool used = false; Seeds seeds; std::vector<ut::Note> pat; bool edited = false; std::vector<std::pair<juce::String, float>> params; };
    std::array<Slot, NumSlots> slots;
};
