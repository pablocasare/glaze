#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", StringArray { "Tempo", "Sidechain" }, 0));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate", pump::rateNames, 2));
    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit)
    { v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def, AudioParameterFloatAttributes().withLabel (unit))); };
    fl ("depth",  "Depth", 0, 100, 85, "%");
    fl ("length", "Length", 5, 100, 55, "%");
    fl ("curve",  "Curve", -100, 100, 35, "");
    fl ("thresh", "Threshold", -60, 0, -24, "dB");
    fl ("mix",    "Mix", 0, 100, 100, "%");
    return { v.begin(), v.end() };
}

PumpProcessor::PumpProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "PUMP", layout())
{
}

bool PumpProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (in != out) return false;
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.inputBuses[1];
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}

void PumpProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    freePhase = 0; scPhase = 1; scEnv = 0; gain = 1; armed = true; lockout = 0;
}

void PumpProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int num = buffer.getNumSamples();
    auto main = getBusBuffer (buffer, false, 0);

    juce::AudioBuffer<float> sc;
    bool scOk = false;
    if (getBusCount (true) > 1)
        if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled() && bus->getNumberOfChannels() > 0)
        {
            sc = getBusBuffer (buffer, true, 1);
            scOk = sc.getNumChannels() > 0;
        }

    // ---- tempo / transport ----
    bool playing = false; double ppq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm.store ((float) *b);
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q; else playing = false;
        }
    const double beatsPerSample = juce::jlimit (20.0f, 400.0f, bpm.load()) / 60.0 / sr;

    const bool sidechainMode = p ("mode") > 0.5f;
    const double cycleBeats = pump::rateBeats[juce::jlimit (0, 5, (int) p ("rate"))];
    const float depth = p ("depth") * 0.01f, length = p ("length") * 0.01f, curve = p ("curve") * 0.01f;
    const float thresh = p ("thresh"), mix = p ("mix") * 0.01f;

    const float envAtt = std::exp (-1.0f / (float) (0.0002 * sr)), envRel = std::exp (-1.0f / (float) (0.040 * sr));
    const float gDown = std::exp (-1.0f / (float) (0.0008 * sr)), gUp = std::exp (-1.0f / (float) (0.0015 * sr));
    const float threshLin = juce::Decibels::decibelsToGain (thresh);
    const float rearmLin = juce::Decibels::decibelsToGain (thresh - 6.0f);

    float scPeak = 0, phaseOut = 0;
    for (int i = 0; i < num; ++i)
    {
        double phase;
        if (! sidechainMode)
        {
            if (playing) phase = std::fmod ((ppq + i * beatsPerSample) / cycleBeats, 1.0);
            else { phase = freePhase; freePhase = std::fmod (freePhase + beatsPerSample / cycleBeats, 1.0); }
            if (phase < 0) phase += 1.0;
        }
        else
        {
            float x = 0;
            if (scOk) for (int c = 0; c < sc.getNumChannels(); ++c) x = std::max (x, std::abs (sc.getSample (c, i)));
            scPeak = std::max (scPeak, x);
            scEnv = x > scEnv ? x + (scEnv - x) * envAtt : x + (scEnv - x) * envRel;
            if (lockout > 0) --lockout;
            if (armed && lockout == 0 && scEnv > threshLin) { scPhase = 0; armed = false; lockout = (int) (0.03 * sr); }
            else if (! armed && scEnv < rearmLin) armed = true;
            phase = scPhase;
            scPhase = std::min (1.0, scPhase + beatsPerSample / cycleBeats);
        }

        const float target = phase >= 1.0 ? 1.0f : pump::shapeGain ((float) phase, depth, length, curve);
        gain = target + (gain - target) * (target < gain ? gDown : gUp);
        const float g = 1.0f - mix + mix * gain;
        for (int c = 0; c < main.getNumChannels(); ++c) main.setSample (c, i, main.getSample (c, i) * g);
        phaseOut = (float) phase;
    }

    uiPhase.store (phaseOut);
    uiGain.store (gain);
    uiScConnected.store (scOk);
    uiScLevelDb.store (juce::Decibels::gainToDecibels (scPeak, -100.0f));
}

void PumpProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}
void PumpProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* PumpProcessor::createEditor() { return new PumpEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PumpProcessor(); }
