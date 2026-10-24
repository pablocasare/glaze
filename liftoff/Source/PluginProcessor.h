#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace liftoff
{
inline const juce::StringArray delayNames { "1/4", "1/8", "1/8 D", "1/16", "1/4 D", "1/8 T" };
inline const float delayBeats[] { 1.0f, 0.5f, 0.75f, 0.25f, 1.5f, 1.0f / 3.0f };

// Where the low-cut sits for a given build (0..1) and filter amount (0..1).
inline float cutoffFor (float build, float amount) { return 20.0f * std::pow (2.0f, std::pow (build, 1.4f) * amount * 7.2f); }
inline float qFor (float build, float res) { return 0.707f + std::pow (build, 1.5f) * res * 5.5f; }
}

class LiftoffProcessor : public juce::AudioProcessor
{
public:
    LiftoffProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Liftoff"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // UI telemetry
    std::atomic<float> uiBuild { 0 }, uiCutoff { 20 }, uiQ { 0.707f }, uiLevel { 0 }, bpm { 120 };

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool on (const char* id) const { return p (id) > 0.5f; }

    double sr = 44100;
    int maxBlock = 512;
    juce::dsp::StateVariableTPTFilter<float> hp, noiseBp;
    std::vector<float> dlL, dlR; int dlW = 0;
    float dlTimeSmoothed = -1;
    float fbHpL = 0, fbHpR = 0;
    juce::Reverb verb;
    juce::AudioBuffer<float> wet;
    juce::Random rng;
    float buildSmoothed = 0, outSmoothed = 1;
    int silentCount = 0;
    bool tailsCleared = true;
};
