#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit)
    { v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def, AudioParameterFloatAttributes().withLabel (unit))); };
    auto bo = [&] (const char* id, const char* name)
    { v.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, true)); };
    fl ("build",     "Build", 0, 100, 0, "%");
    fl ("filter",    "Filter", 0, 100, 100, "%");
    fl ("resonance", "Resonance", 0, 100, 50, "%");
    fl ("delay",     "Delay", 0, 100, 60, "%");
    fl ("reverb",    "Reverb", 0, 100, 70, "%");
    fl ("noise",     "Noise", 0, 100, 35, "%");
    fl ("output",    "Output", -24, 12, 0, "dB");
    bo ("filterOn", "Filter On"); bo ("resOn", "Resonance On"); bo ("delayOn", "Delay On");
    bo ("reverbOn", "Reverb On"); bo ("noiseOn", "Noise On");
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "delayTime", 1 }, "Delay Time", liftoff::delayNames, 0));
    return { v.begin(), v.end() };
}

LiftoffProcessor::LiftoffProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LIFTOFF", layout())
{
}

bool LiftoffProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())
        && (l.getMainInputChannelSet() == out || l.getMainInputChannelSet() == juce::AudioChannelSet::mono());
}

void LiftoffProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, 2 };
    hp.prepare (spec); hp.setType (juce::dsp::StateVariableTPTFilterType::highpass); hp.reset();
    noiseBp.prepare (spec); noiseBp.setType (juce::dsp::StateVariableTPTFilterType::bandpass); noiseBp.reset();
    const int dlSize = (int) (sr * 4.0) + 8;
    dlL.assign ((size_t) dlSize, 0.0f); dlR.assign ((size_t) dlSize, 0.0f); dlW = 0;
    dlTimeSmoothed = -1; fbHpL = fbHpR = 0;
    verb.setSampleRate (sr); verb.reset();
    wet.setSize (2, maxBlock);
    buildSmoothed = p ("build") * 0.01f;
    outSmoothed = juce::Decibels::decibelsToGain (p ("output"));
    tailsCleared = false;
}

void LiftoffProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int num = buffer.getNumSamples();
    if (num == 0) return;
    if (num > maxBlock)
    {
        for (int s = 0; s < num; s += maxBlock)
        {
            juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), s, std::min (maxBlock, num - s));
            juce::MidiBuffer m; processBlock (sub, m);
        }
        return;
    }

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm.store ((float) *b);

    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    float* L = buffer.getWritePointer (0);
    float* R = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    if (R != nullptr && numIn < 2) std::copy (L, L + num, R); // mono in -> stereo out

    const float buildTarget = p ("build") * 0.01f;
    const float filterAmt = on ("filterOn") ? p ("filter") * 0.01f : 0.0f;
    const float resAmt    = on ("resOn") ? p ("resonance") * 0.01f : 0.0f;
    const float delayAmt  = on ("delayOn") ? p ("delay") * 0.01f : 0.0f;
    const float verbAmt   = on ("reverbOn") ? p ("reverb") * 0.01f : 0.0f;
    const float noiseAmt  = on ("noiseOn") ? p ("noise") * 0.01f : 0.0f;
    const float outTarget = juce::Decibels::decibelsToGain (p ("output"));
    const float dlTarget = juce::jlimit (1.0f, (float) (sr * 3.9), liftoff::delayBeats[juce::jlimit (0, 5, (int) p ("delayTime"))]
                                                                    * 60.0f / juce::jlimit (40.0f, 300.0f, bpm.load()) * (float) sr);
    if (dlTimeSmoothed < 0) dlTimeSmoothed = dlTarget;

    const float bSmooth = 1.0f - std::exp (-1.0f / (float) (0.015 * sr));
    const int dlSize = (int) dlL.size();
    auto* wl = wet.getWritePointer (0);
    auto* wr = wet.getWritePointer (1);

    float peak = 0, cutoff = 20, q = 0.707f;
    for (int i = 0; i < num; ++i)
    {
        buildSmoothed += (buildTarget - buildSmoothed) * bSmooth;
        outSmoothed += (outTarget - outSmoothed) * 0.002f;
        const float b = buildSmoothed;

        if ((i & 15) == 0)
        {
            cutoff = juce::jlimit (20.0f, (float) (sr * 0.45), liftoff::cutoffFor (b, filterAmt));
            q = liftoff::qFor (b, resAmt);
            hp.setCutoffFrequency (cutoff);
            hp.setResonance (q);
            noiseBp.setCutoffFrequency (juce::jlimit (200.0f, (float) (sr * 0.4), 400.0f * std::pow (2.0f, b * 4.6f)));
            noiseBp.setResonance (1.2f);
        }

        float l = L[i], r = R ? R[i] : L[i];
        // white-noise riser
        if (noiseAmt > 0.0f && b > 0.001f)
        {
            const float lvl = b * b * noiseAmt * 0.22f;
            l += noiseBp.processSample (0, rng.nextFloat() * 2.0f - 1.0f) * lvl;
            r += noiseBp.processSample (1, rng.nextFloat() * 2.0f - 1.0f) * lvl;
        }
        if (filterAmt > 0.0f || resAmt > 0.0f)
        {
            l = hp.processSample (0, l);
            r = hp.processSample (1, r);
        }

        // ping-pong delay, feedback grows with the build
        dlTimeSmoothed += (dlTarget - dlTimeSmoothed) * 0.0005f;
        const float fb = 0.25f + 0.5f * b * delayAmt;
        auto readAt = [&] (const std::vector<float>& d)
        {
            const float pos = (float) dlW - dlTimeSmoothed;
            float fp = pos < 0 ? pos + (float) dlSize : pos;
            const int i0 = (int) fp; const float fr = fp - (float) i0;
            const int i1 = (i0 + 1) % dlSize;
            return d[(size_t) i0] + (d[(size_t) i1] - d[(size_t) i0]) * fr;
        };
        const float dL = readAt (dlL), dR = readAt (dlR);
        // keep the delay thin: one-pole low cut in the feedback path
        const float hc = std::exp (-2.0f * juce::MathConstants<float>::pi * juce::jmax (250.0f, cutoff) / (float) sr);
        fbHpL = dR + (fbHpL - dR) * hc; fbHpR = dL + (fbHpR - dL) * hc;
        const float thinL = dR - fbHpL, thinR = dL - fbHpR;
        dlL[(size_t) dlW] = 0.5f * (l + r) * delayAmt + thinL * fb;
        dlR[(size_t) dlW] = thinR * fb;
        dlW = (dlW + 1) % dlSize;

        const float dWet = b * delayAmt * 0.65f;
        wl[i] = l; wr[i] = r;                 // reverb input (post-filter)
        L[i] = l + dL * dWet;
        if (R) R[i] = r + dR * dWet;
    }

    // reverb grows with the build
    {
        juce::Reverb::Parameters rp;
        rp.roomSize = 0.55f + 0.43f * buildSmoothed;
        rp.damping = 0.35f; rp.width = 1.0f; rp.wetLevel = 1.0f; rp.dryLevel = 0.0f;
        verb.setParameters (rp);
        verb.processStereo (wl, wr, num);
        const float vWet = buildSmoothed * verbAmt * 0.55f;
        for (int i = 0; i < num; ++i)
        {
            L[i] = (L[i] + wl[i] * vWet) * outSmoothed;
            if (R) R[i] = (R[i] + wr[i] * vWet) * outSmoothed;
            peak = std::max (peak, std::abs (L[i]));
        }
    }

    // clean drop: once the build is back at zero, kill the tails so the drop hits dry
    if (buildSmoothed < 0.003f)
    {
        if (! tailsCleared && ++silentCount > 3)
        {
            std::fill (dlL.begin(), dlL.end(), 0.0f); std::fill (dlR.begin(), dlR.end(), 0.0f);
            verb.reset(); noiseBp.reset();
            tailsCleared = true;
        }
    }
    else { silentCount = 0; tailsCleared = false; }

    for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, num);
    uiBuild.store (buildSmoothed);
    uiCutoff.store (cutoff);
    uiQ.store (q);
    uiLevel.store (peak);
}

void LiftoffProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, d);
}
void LiftoffProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* LiftoffProcessor::createEditor() { return new LiftoffEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LiftoffProcessor(); }
