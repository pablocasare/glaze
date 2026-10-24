#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP.h"

class GlazeProcessor : public juce::AudioProcessor
{
public:
    GlazeProcessor();
    ~GlazeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Glaze"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // ---- UI telemetry ----
    glaze::ScopeBuffer inScope, outScope;
    std::atomic<float> gateEnvDb { -120 }, gateGainDb { 0 }, compInDb { -120 }, compGrDb { 0 },
                       dsGrDb { 0 }, limGrDb { 0 }, outPeakDb { -120 }, inPeakDb { -120 }, bpm { 120 };
    glaze::Tuner& getTuner() { return tuner; }
    double getSampleRateSafe() const { return sr; }
    int currentPreset = 0;

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool  b (const char* id) const { return p (id) > 0.5f; }
    void updateLatency();

    double sr = 44100;
    int maxBlock = 512;
    glaze::Tuner tuner;
    glaze::Gate gate;
    glaze::Biquad eqLow1, eqLow2, eqBody, eqPres, eqAir;
    glaze::Compressor comp;
    glaze::DeEsser deess;
    glaze::Drive drive;
    glaze::Doubler doubler;
    glaze::Chorus chorus;
    glaze::StereoDelay delay;
    glaze::Verb verb;
    glaze::Limiter limiter;
    glaze::DelayLine dryL, dryR;
    juce::AudioBuffer<float> mono, stereo;
    bool lastLowLatency = false;
    int reportedLatency = -1;
    float eqCache[6] { -1, -1, -1, -1, -1, -1 };
    float smoothIn = 1, smoothOut = 1, smoothMix = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlazeProcessor)
};
