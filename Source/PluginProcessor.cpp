#include "PluginProcessor.h"
#if ! GLAZE_HEADLESS
 #include "PluginEditor.h"
#endif

using namespace glaze;

GlazeProcessor::GlazeProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GLAZE", createLayout())
{
}

bool GlazeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    const auto in  = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    if (in  != juce::AudioChannelSet::stereo() && in  != juce::AudioChannelSet::mono()) return false;
    return true;
}

void GlazeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (1, samplesPerBlock);
    lastLowLatency = b ("lowLatency");

    tuner.prepare (sr, lastLowLatency);
    gate.prepare (sr); gate.setup();
    comp.prepare (sr);
    deess.prepare (sr); deess.setup();
    drive.prepare (sr);
    doubler.prepare (sr);
    chorus.prepare (sr);
    delay.prepare (sr);
    verb.prepare (sr, maxBlock); verb.reset();
    limiter.prepare (sr);
    dryL.prepare ((int) (sr * 0.06)); dryR.prepare ((int) (sr * 0.06));
    for (auto* f : { &eqLow1, &eqLow2, &eqBody, &eqPres, &eqAir }) f->reset();
    std::fill (std::begin (eqCache), std::end (eqCache), -1.0f);

    mono.setSize (1, maxBlock);
    stereo.setSize (2, maxBlock);
    reportedLatency = -1;
    updateLatency();
}

void GlazeProcessor::updateLatency()
{
    const int lat = tuner.getLatency() + limiter.lookahead (lastLowLatency);
    if (lat != reportedLatency) { reportedLatency = lat; setLatencySamples (lat); }
}

void GlazeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int num = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    if (num == 0) return;

    if (num > maxBlock)
    {
        // Host sent a bigger block than promised: process in chunks.
        for (int start = 0; start < num; start += maxBlock)
        {
            const int len = std::min (maxBlock, num - start);
            juce::AudioBuffer<float> sub (buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, len);
            juce::MidiBuffer m;
            processBlock (sub, m);
        }
        return;
    }

    // ---- latency mode ----
    const bool ll = b ("lowLatency");
    if (ll != lastLowLatency)
    {
        lastLowLatency = ll;
        tuner.setLowLatency (ll);
        tuner.reset();
        updateLatency();
    }

    // ---- tempo ----
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpmOpt = pos->getBpm())
                bpm.store ((float) *bpmOpt);

    // ---- parameters ----
    const float inG = dbToGain (p ("inGain")), outG = dbToGain (p ("outGain"));
    const float mixAmt = p ("mix") * 0.01f;
    const float punch = p ("punch") * 0.01f, space = p ("space") * 0.01f, width = p ("width") * 0.01f;

    Tuner::Settings ts;
    ts.enabled = b ("tuneOn");
    ts.amount = p ("tuneAmount") * 0.01f;
    ts.speedMs = p ("tuneSpeed");
    ts.humanize = p ("humanize") * 0.01f;
    ts.formantSt = ts.enabled ? p ("formant") : 0.0f;
    ts.transposeSt = ts.enabled ? p ("transpose") : 0.0f;
    ts.key = (int) p ("key");
    ts.scale = (int) p ("scale");

    const bool gateOn = b ("gateOn");
    const float gThr = p ("gateThresh"), gRange = p ("gateRange"), gAtt = p ("gateAttack"), gRel = p ("gateRelease");

    const bool eqOn = b ("eqOn");
    {
        const float vals[6] { p ("eqLowCut"), p ("eqBody"), p ("eqBodyFreq"), p ("eqPres"), p ("eqPresFreq"), p ("eqAir") };
        if (! std::equal (std::begin (vals), std::end (vals), std::begin (eqCache)))
        {
            std::copy (std::begin (vals), std::end (vals), std::begin (eqCache));
            eqLow1.set (Biquad::HighPass, sr, vals[0], 0.54f);
            eqLow2.set (Biquad::HighPass, sr, vals[0], 1.31f);
            eqBody.set (Biquad::Peak, sr, vals[2], 0.8f, vals[1]);
            eqPres.set (Biquad::Peak, sr, vals[4], 0.9f, vals[3]);
            eqAir.set (Biquad::HighShelf, sr, 10000.0f, 0.7f, vals[5]);
        }
    }

    const bool compOn = b ("compOn");
    const float cThr = p ("compThresh"), cRatio = p ("compRatio"), cAtt = p ("compAttack"), cRel = p ("compRelease"),
                cMake = p ("compMakeup"), cMix = p ("compMix") * 0.01f;

    const bool dsOn = b ("dsOn");
    deess.setFreq (p ("dsFreq"));
    const float dsThr = p ("dsThresh"), dsRange = p ("dsRange");

    const bool drvOn = b ("drvOn");
    const float drvAmt = p ("drvAmount") * 0.01f, drvTone = p ("drvTone") * 0.01f, drvMix = p ("drvMix") * 0.01f;

    const bool dblOn = b ("dblOn");
    const float dblMix = juce::jlimit (0.0f, 1.0f, p ("dblMix") * 0.01f * width), dblDet = p ("dblDetune"), dblDel = p ("dblDelay");

    const bool chOn = b ("chOn");
    const float chRate = p ("chRate"), chDepth = p ("chDepth") * 0.01f, chMix = p ("chMix") * 0.01f;

    const bool dlOn = b ("dlOn");
    float dlTime = p ("dlTime");
    if (b ("dlSync"))
        dlTime = divBeats[juce::jlimit (0, 7, (int) p ("dlDiv"))] * 60000.0f / juce::jlimit (30.0f, 300.0f, bpm.load());
    const float dlFb = p ("dlFeedback") * 0.01f, dlMix = juce::jlimit (0.0f, 1.0f, p ("dlMix") * 0.01f * space);
    const bool dlPing = b ("dlPingPong");
    delay.setFilters (p ("dlLowCut"), p ("dlHighCut"));

    const bool rvOn = b ("rvOn");
    verb.setParams (p ("rvSize") * 0.01f, p ("rvDamp") * 0.01f, p ("rvWidth") * 0.01f, p ("rvLowCut"));
    const float rvMix = juce::jlimit (0.0f, 1.5f, p ("rvMix") * 0.01f * space), rvPre = p ("rvPreDelay");

    const bool limOn = b ("limOn");
    const float limGain = p ("limGain"), limCeil = p ("limCeiling"), limRel = p ("limRelease");

    // ---- mono vocal chain ----
    const float* inL = buffer.getReadPointer (0);
    const float* inR = numIn > 1 ? buffer.getReadPointer (1) : inL;
    auto* m = mono.getWritePointer (0);
    auto* L = stereo.getWritePointer (0);
    auto* R = stereo.getWritePointer (1);
    const int dryDelay = tuner.getLatency();

    float inPk = 0;
    for (int i = 0; i < num; ++i)
    {
        smoothIn += (inG - smoothIn) * 0.002f;
        dryL.push (inL[i]); dryR.push (inR[i]);
        float x = 0.5f * (inL[i] + inR[i]) * smoothIn;
        inPk = std::max (inPk, std::abs (x));
        inScope.push (x, x);

        if (gateOn) x = gate.process (x, gThr, gRange, gAtt, gRel);
        else { gate.process (x, gThr, gRange, gAtt, gRel); gate.gain = 1; }

        x = tuner.process (x, ts);

        if (eqOn)
        {
            x = eqLow2.process (eqLow1.process (x));
            x = eqAir.process (eqPres.process (eqBody.process (x)));
        }
        if (compOn) x = comp.process (x, cThr, cRatio, cAtt, cRel, cMake, cMix, punch);
        else comp.grDb = 0;
        if (dsOn) x = deess.process (x, dsThr, dsRange);
        else deess.grDb = 0;
        if (drvOn) x = drive.process (x, drvAmt, drvTone, drvMix);
        m[i] = x;

        // ---- stereo section ----
        float l = x, r = x;
        if (dblOn) doubler.process (x, l, r, dblMix, dblDet, dblDel);
        if (chOn) chorus.process (l, r, chRate, chDepth, chMix);
        if (dlOn) delay.process (l, r, dlTime, dlFb, dlMix, dlPing);
        L[i] = l; R[i] = r;
    }

    if (rvOn) verb.process (L, R, num, rvPre, rvMix);

    // ---- global mix, limiter, output ----
    float outPk = 0;
    auto* oL = buffer.getWritePointer (0);
    auto* oR = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < num; ++i)
    {
        smoothMix += (mixAmt - smoothMix) * 0.002f;
        smoothOut += (outG - smoothOut) * 0.002f;
        const float dl = dryL.read ((float) dryDelay + 1.0f) * smoothIn;
        const float dr = dryR.read ((float) dryDelay + 1.0f) * smoothIn;
        float l = dl + (L[i] - dl) * smoothMix;
        float r = dr + (R[i] - dr) * smoothMix;
        if (limOn) limiter.process (l, r, limGain, limCeil, limRel, lastLowLatency);
        else
        {
            // keep latency constant even when the limiter is off
            float a = l, c = r; limiter.process (a, c, 0, 24, limRel, lastLowLatency);
            if (! lastLowLatency) { l = a; r = c; }
            limiter.grDb = 0;
        }
        l *= smoothOut; r *= smoothOut;
        outScope.push (l, r);
        outPk = std::max (outPk, std::max (std::abs (l), std::abs (r)));
        if (oR) { oL[i] = l; oR[i] = r; }
        else oL[i] = 0.5f * (l + r);
    }
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, num);

    // ---- telemetry ----
    inPeakDb.store (gainToDb (inPk));
    outPeakDb.store (gainToDb (outPk));
    gateEnvDb.store (gainToDb (gate.env));
    gateGainDb.store (gainToDb (gate.gain));
    compInDb.store (comp.env);
    compGrDb.store (comp.grDb);
    dsGrDb.store (deess.grDb);
    limGrDb.store (limiter.grDb);
}

// ---- presets ---------------------------------------------------------------
int GlazeProcessor::getNumPrograms() { return (int) factoryPresets().size(); }
const juce::String GlazeProcessor::getProgramName (int i)
{
    return juce::isPositiveAndBelow (i, getNumPrograms()) ? juce::String (factoryPresets()[(size_t) i].name) : juce::String();
}

void GlazeProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms())) return;
    currentPreset = index;
    // reset everything (except key/scale, which belong to the song) to defaults, then apply preset
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto id = rp->getParameterID();
            if (id == "key" || id == "scale") continue;
            rp->setValueNotifyingHost (rp->getDefaultValue());
        }
    for (auto& [id, v] : factoryPresets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

void GlazeProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void GlazeProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = vt.getProperty ("preset", 0);
            apvts.replaceState (vt);
        }
}

#if ! GLAZE_HEADLESS
juce::AudioProcessorEditor* GlazeProcessor::createEditor() { return new GlazeEditor (*this); }
#else
juce::AudioProcessorEditor* GlazeProcessor::createEditor() { return nullptr; }
#endif

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GlazeProcessor(); }
