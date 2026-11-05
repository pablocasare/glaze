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
    double phase = 0, phase2 = 0, subPhase = 0;
    float freq = 55, target = 55, glideCoef = 1;
    float amp = 0, ampTarget = 0, fenv = 0;
    bool gate = false; int note = -1; float vel = 1;
    float ic1 = 0, ic2 = 0; // SVF state
    void noteOn (int n, float v, bool slide, float glideMs);
    void noteOff (int n) { if (n == note) gate = false; }
    void allOff() { gate = false; note = -1; }
    float render (int sound, float cutoffOffset, float drive);
};

struct Kick
{
    double sr = 44100, t = 1e9, ph = 0;
    void trig() { t = 0; ph = 0; }
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
    int currentPreset = 0;
    bool presetModified = false;
    std::vector<ut::Note> getPattern() const { const juce::SpinLock::ScopedLockType l (patLock); return pattern; }
    ut::Params currentParams() const;
    int rootMidi() const;
    int scale() const { return (int) p ("scale"); }
    int bars() const { return ut::barVals[(int) p ("bars")]; }
    float swing() const { return p ("swing"); }
    bool writeMidi (const juce::File& f) const;
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

    struct Seeds { uint32_t r = 11, p = 7, m = 0; int muts = 0; };
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
    double loopBeats = 8;
    void rebuildEvents();

    ut::Voice voice; ut::Kick kick;
    double sr = 44100, internalPpq = 0; bool wasPlaying = false;
    int sounding = -1;
    float duck = 1;
};
