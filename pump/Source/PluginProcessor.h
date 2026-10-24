#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <cmath>

namespace pump
{
inline const juce::StringArray rateNames { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars" };
inline const float rateBeats[] { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f };

// Gain (0..1) of the duck shape at phase p (0..1 through one cycle).
inline float shapeGain (float p, float depth, float length, float curve)
{
    // short fade-in of the duck so the dip itself never clicks
    const float attack = 0.004f;
    float r;
    if (p < attack) r = 1.0f - p / attack;
    else
    {
        const float q = (p - attack) / std::max (0.01f, length - attack);
        const float k = std::pow (2.0f, curve * 2.5f); // >1: holds down longer (more pump), <1: snaps back
        r = q >= 1.0f ? 1.0f : std::pow (q, k);
    }
    return 1.0f - depth * (1.0f - r);
}
}

class PumpProcessor : public juce::AudioProcessor
{
public:
    PumpProcessor();
    void prepareToPlay (double sampleRate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Pump"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    // UI telemetry
    std::atomic<float> uiPhase { 0 }, uiGain { 1 }, uiScLevelDb { -100 }, bpm { 120 };
    std::atomic<bool> uiScConnected { false };

private:
    float p (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    double sr = 44100;
    double freePhase = 0;     // tempo mode when transport is stopped
    double scPhase = 1;       // sidechain mode: 1 = idle (no duck)
    float scEnv = 0, gain = 1;
    bool armed = true;
    int lockout = 0;
};
