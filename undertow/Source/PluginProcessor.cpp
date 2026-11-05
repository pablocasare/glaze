#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ut;

// ---------------------------------------------------------------- synth
static inline float polyBlep (double t, double dt)
{
    if (t < dt) { t /= dt; return (float) (t + t - t * t - 1.0); }
    if (t > 1.0 - dt) { t = (t - 1.0) / dt; return (float) (t * t + t + t + 1.0); }
    return 0.f;
}

void Voice::noteOn (int n, float v, bool slide, float glideMs)
{
    target = 440.f * std::pow (2.f, (n - 69) / 12.f);
    const bool legato = gate && slide;
    if (legato) glideCoef = 1.f - std::exp (-1.f / (float) (std::max (5.f, glideMs) * 0.001f * sr));
    else { freq = target; glideCoef = 1.f; fenv = 1.f; }
    note = n; vel = v; gate = true;
    // no hard amp reset on retrigger: the 22 ms release + 1.5 ms attack handle it without clicks
}

float Voice::render (int sound, float cutOff, float drive)
{
    // per-sound recipe: sub, saw, square, saw2 detune, base cutoff (Hz), reso, env amount (oct), env decay (s), drive
    struct R { float sub, saw, sq, det, cut, res, env, dec, drv; };
    static const R rec[] = {
        { 1.0f, 0.12f, 0.0f, 0.0f, 260, 0.2f, 1.2f, 0.10f, 0.15f },  // Sub
        { 0.7f, 0.0f, 0.55f, 0.0f, 380, 0.45f, 3.2f, 0.09f, 0.35f }, // Rubber
        { 0.6f, 0.55f, 0.0f, 0.12f, 650, 0.25f, 1.6f, 0.25f, 0.6f }, // Reese
        { 0.9f, 0.0f, 0.0f, 0.0f, 2200, 0.1f, 0.0f, 0.1f, 0.25f },   // Organ (additive below)
        { 0.35f, 0.85f, 0.0f, 0.0f, 300, 0.85f, 4.2f, 0.16f, 0.5f }, // Acid
        { 0.6f, 0.4f, 0.4f, 0.04f, 520, 0.5f, 2.5f, 0.14f, 0.9f } }; // Growl
    const R& r = rec[juce::jlimit (0, 5, sound)];

    freq += (target - freq) * glideCoef;
    const double dt = freq / sr;
    phase += dt; if (phase >= 1) phase -= 1;
    phase2 += dt * (1.0 + r.det * 0.06); if (phase2 >= 1) phase2 -= 1;
    subPhase += dt; if (subPhase >= 1) subPhase -= 1;

    float x = r.sub * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase);
    if (r.saw > 0)
    {
        float s = (float) (2 * phase - 1) - polyBlep (phase, dt);
        if (r.det > 0) s = 0.5f * (s + (float) (2 * phase2 - 1) - polyBlep (phase2, dt * (1.0 + r.det * 0.06)));
        x += r.saw * s;
    }
    if (r.sq > 0)
    {
        float q = phase < 0.5 ? 1.f : -1.f;
        q += polyBlep (phase, dt); double p2 = phase + 0.5; if (p2 >= 1) p2 -= 1; q -= polyBlep (p2, dt);
        x += r.sq * q;
    }
    if (sound == 3) // organ: drawbar-ish 2nd + 3rd harmonic
        x += 0.35f * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 2) + 0.18f * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 3);

    // envelopes
    ampTarget = gate ? 1.f : 0.f;
    const float aC = gate ? (1.f - std::exp (-1.f / (0.0015f * (float) sr))) : (1.f - std::exp (-1.f / (0.022f * (float) sr)));
    amp += (ampTarget - amp) * aC;
    fenv *= std::exp (-1.f / (r.dec * (float) sr));

    // filter (Cytomic SVF lowpass)
    float fc = r.cut * std::pow (2.f, cutOff * 2.5f + r.env * fenv * (0.5f + 0.5f * vel));
    fc = juce::jlimit (30.f, (float) sr * 0.45f, fc);
    const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
    const float k = 2.f - 1.9f * r.res;
    const float a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
    float y = v2;
    const float d = 1.f + (r.drv + drive) * 4.f;
    y = std::tanh (y * d) / std::tanh (d) * 0.9f;
    return y * amp * (0.55f + 0.45f * vel);
}

float Kick::render()
{
    if (t > 0.5) return 0.f;
    const double f = 48 + 120 * std::exp (-t / 0.035);
    ph += f / sr;
    float y = (float) std::sin (juce::MathConstants<double>::twoPi * ph) * (float) std::exp (-t / 0.22);
    y += (t < 0.003 ? 0.4f * (float) (1 - t / 0.003) : 0.f);
    t += 1.0 / sr;
    return std::tanh (1.6f * y) * 0.7f;
}

// ---------------------------------------------------------------- processor
static juce::AudioProcessorValueTreeState::ParameterLayout layout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> v;
    StringArray styles, scales;
    for (int i = 0; i < NumStyles; ++i) styles.add (styleName (i));
    for (int i = 0; i < NumScales; ++i) scales.add (scaleName (i));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", styles, 0));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keyNames, 9));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scales, 0));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "octave", 1 }, "Octave", StringArray { "1", "2", "3" }, 0));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Bars", barNames, 2));
    StringArray chordNames; for (int i = 0; i < NumChords; ++i) chordNames.add (chordName (i));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "chords", 1 }, "Chords", chordNames, 0));
    auto pct = [&] (const char* id, const char* nm, float def) {
        v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, nm, NormalisableRange<float> (0, 1), def,
            AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x * 100)) + "%"; })));
    };
    pct ("density", "Density", 0.6f);
    pct ("movement", "Movement", 0.45f);
    pct ("variation", "Variation", 0.3f);
    pct ("gate", "Length", 0.8f);
    pct ("ghosts", "Ghost Notes", 0.3f);
    pct ("accent", "Accent", 0.5f);
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (0.5f, 0.75f), 0.56f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x * 100)) + "%"; })));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "glide", 1 }, "Glide", NormalisableRange<float> (5, 250, 0, 0.6f), 60,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x)) + " ms"; })));
    v.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "sound", 1 }, "Sound", soundNames, 1));
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tone", 1 }, "Tone", NormalisableRange<float> (-1, 1), 0,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (roundToInt (x * 100)); })));
    pct ("drive", "Drive", 0.2f);
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume", NormalisableRange<float> (-30, 6), -6,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float x, int) { return String (x, 1) + " dB"; })));
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "kick", 1 }, "Kick Preview", true));
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "lockRhythm", 1 }, "Lock Rhythm", false));
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "lockPitch", 1 }, "Lock Pitch", false));
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "evolve", 1 }, "Evolve", true));
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "mute", 1 }, "Mute Sound (MIDI only)", false));
    return { v.begin(), v.end() };
}

static const char* genIds[] = { "style", "key", "scale", "octave", "bars", "density", "movement", "variation", "gate", "swing", "chords", "ghosts", "accent" };

UndertowProcessor::UndertowProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "UNDERTOW", layout())
{
    for (auto* id : genIds) apvts.addParameterListener (id, this);
    regenerate();
    startTimerHz (30);
}

UndertowProcessor::~UndertowProcessor() { for (auto* id : genIds) apvts.removeParameterListener (id, this); }

// key, scale, octave, swing and length only change how the notes play, so they never throw away hand edits
void UndertowProcessor::parameterChanged (const juce::String& id, float)
{
    if (! applyingPreset) presetModified = true;
    if (id == "key" || id == "scale" || id == "octave" || id == "swing" || id == "gate") renderDirty = true;
    else dirty = true;
}

const std::vector<UndertowProcessor::Preset>& UndertowProcessor::presets()
{
    using namespace ut;
    static const std::vector<Preset> P {
        { "Night Roller",       Rolling,     Minor,     ChordAuto,         .65f, .40f, .30f, .80f, .56f,  60, .30f, .50f, 1,  .00f, .20f },
        { "Warehouse Driver",   Driving,     Minor,     ChordStay,         .70f, .30f, .20f, .60f, .52f,  40, .20f, .60f, 5,  .10f, .40f },
        { "Offbeat Classic",    Offbeat,     Dorian,    ChordI_VII,        .50f, .35f, .25f, .85f, .54f,  60, .15f, .45f, 3,  .00f, .15f },
        { "Bouncy Tech",        Bouncy,      Minor,     ChordI_VI,         .55f, .50f, .35f, .50f, .56f,  50, .35f, .60f, 1,  .15f, .25f },
        { "Garage Swing",       TwoStep,     Minor,     ChordI_VI_VII,     .60f, .50f, .35f, .85f, .66f,  90, .30f, .55f, 2,  .00f, .35f },
        { "Minimal Hypnosis",   Minimal,     Phrygian,  ChordStay,         .40f, .20f, .20f, .60f, .58f,  40, .25f, .40f, 0, -.10f, .10f },
        { "Off The Grid",       Broken,      Minor,     ChordI_iv,         .60f, .55f, .45f, .60f, .57f,  60, .35f, .55f, 1,  .10f, .30f },
        { "Squelch 303",        Acid,        Phrygian,  ChordStay,         .70f, .60f, .40f, .70f, .54f,  70, .20f, .70f, 4,  .20f, .45f },
        { "Speed Wobble",       SpeedGarage, Minor,     ChordI_VII,        .65f, .50f, .35f, .80f, .60f,  80, .25f, .55f, 2,  .10f, .50f },
        { "Tresillo Sun",       Afro,        Dorian,    ChordI_iv,         .55f, .45f, .30f, .75f, .55f,  50, .35f, .50f, 0,  .05f, .15f },
        { "Deep Undertow",      DeepTech,    Minor,     ChordI_VI,         .45f, .35f, .25f, .95f, .56f, 120, .20f, .40f, 0, -.15f, .20f },
        { "Bassline Bounce",    Bassline,    Minor,     ChordI_iv_VI_V,    .65f, .55f, .30f, .55f, .52f,  50, .25f, .65f, 5,  .20f, .45f },
        { "Late Night Reese",   Rolling,     HarmMinor, ChordI_VI_VII,     .55f, .55f, .35f, .90f, .56f, 100, .25f, .50f, 2,  .00f, .30f },
        { "Sub Pressure",       Offbeat,     Minor,     ChordStay,         .45f, .20f, .20f, .90f, .53f,  40, .10f, .40f, 0, -.20f, .10f },
        { "Jackin Organ",       Bouncy,      Dorian,    ChordI_VII,        .60f, .45f, .35f, .55f, .58f,  50, .30f, .60f, 3,  .10f, .20f },
        { "Tunnel Vision",      Driving,     Phrygian,  ChordI_VI,         .75f, .40f, .30f, .50f, .53f,  40, .35f, .70f, 4, -.10f, .50f } };
    return P;
}

void UndertowProcessor::applyPreset (int index)
{
    const auto& L = presets(); if (L.empty()) return;
    index = (index % (int) L.size() + (int) L.size()) % (int) L.size();
    const auto& P = L[(size_t) index];
    pushHistory();
    applyingPreset = true;
    auto set = [&] (const char* id, float v) { if (auto* q = apvts.getParameter (id)) { q->beginChangeGesture(); q->setValueNotifyingHost (q->convertTo0to1 (v)); q->endChangeGesture(); } };
    set ("style", (float) P.style); set ("scale", (float) P.scale); set ("chords", (float) P.chords);
    set ("density", P.density); set ("movement", P.movement); set ("variation", P.variation); set ("gate", P.gate);
    set ("swing", P.swing); set ("glide", P.glide); set ("ghosts", P.ghosts); set ("accent", P.accent);
    set ("sound", (float) P.sound); set ("tone", P.tone); set ("drive", P.drive);
    applyingPreset = false;
    currentPreset = index; presetModified = false;
    edited = false; dirty = true;
}

UndertowProcessor::Snapshot UndertowProcessor::snap() const { Snapshot s; s.seeds = seeds; s.pat = getPattern(); s.edited = edited; return s; }
void UndertowProcessor::restore (const Snapshot& s)
{
    seeds = s.seeds; edited = s.edited;
    { const juce::SpinLock::ScopedLockType l (patLock); pattern = s.pat; rebuildEvents(); }
    patternVersion++;
}
void UndertowProcessor::pushHistory()
{
    history.push_back (snap()); if (history.size() > 100) history.erase (history.begin());
    future.clear();
}
void UndertowProcessor::beginEdit() { pushHistory(); }
void UndertowProcessor::applyEdit (std::vector<ut::Note> notes)
{
    std::sort (notes.begin(), notes.end(), [] (const ut::Note& a, const ut::Note& b) { return a.step < b.step; });
    edited = true;
    { const juce::SpinLock::ScopedLockType l (patLock); pattern = std::move (notes); rebuildEvents(); }
    patternVersion++;
}

bool UndertowProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() || l.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void UndertowProcessor::prepareToPlay (double s, int) { sr = s; voice.sr = s; kick.sr = s; }

ut::Params UndertowProcessor::currentParams() const
{
    Params P;
    P.style = (int) p ("style"); P.bars = bars();
    P.density = p ("density"); P.movement = p ("movement"); P.variation = p ("variation"); P.gate = p ("gate");
    P.chords = (int) p ("chords"); P.ghosts = p ("ghosts"); P.accent = p ("accent");
    P.rhythmSeed = seeds.r; P.pitchSeed = seeds.p; P.take = seeds.take; P.mutateSeed = seeds.m; P.mutations = seeds.muts;
    return P;
}

int UndertowProcessor::rootMidi() const { return 12 * ((int) p ("octave") + 2) + (int) p ("key"); }

void UndertowProcessor::regenerate()
{
    if (edited) { pushHistory(); edited = false; }
    auto pat = generate (currentParams());
    {
        const juce::SpinLock::ScopedLockType l (patLock);
        pattern = std::move (pat);
        rebuildEvents();
    }
    patternVersion++;
}

void UndertowProcessor::rebuildEvents()
{
    events.clear();
    const float sw = swing(); const int root = rootMidi(), sc = scale();
    loopBeats = bars() * 4.0;
    auto beatOf = [&] (int step) { return step * 0.25 + ((step % 2) ? (sw - 0.5) * 0.5 : 0.0); };
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        const auto& n = pattern[i];
        const int midi = juce::jlimit (0, 127, root + degToSemi (n.deg, sc));
        const float gk = 0.3f + 0.67f * p ("gate");
        double on = beatOf (n.step), off = on + std::max (0.06, (beatOf (n.step + n.len) - on) * gk - 0.01);
        const bool legatoNext = i + 1 < pattern.size() && pattern[i + 1].slide && pattern[i + 1].step == n.step + n.len;
        if (legatoNext) off = beatOf (n.step + n.len) + 0.02; // overlap so the next note glides
        events.push_back ({ on, true, midi, n.vel, n.slide, (int) i });
        events.push_back ({ std::fmod (off, loopBeats), false, midi, 0, false, (int) i });
    }
    std::sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat || (a.beat == b.beat && ! a.on && b.on); });
}

void UndertowProcessor::timerCallback()
{
    // EVOLVE: every time the pattern comes around, play a new take on it (same hook, new variations + fills)
    if (wrapped.exchange (false) && p ("evolve") > 0.5f && ! edited && bars() > 1) { seeds.take++; dirty = true; }
    if (dirty.exchange (false)) { renderDirty = false; regenerate(); }
    else if (renderDirty.exchange (false)) { { const juce::SpinLock::ScopedLockType l (patLock); rebuildEvents(); } patternVersion++; }
}

void UndertowProcessor::generateNew()
{
    pushHistory(); edited = false;
    if (p ("lockRhythm") < 0.5f) seeds.r = (uint32_t) rnd.nextInt();
    if (p ("lockPitch") < 0.5f) seeds.p = (uint32_t) rnd.nextInt();
    seeds.muts = 0; seeds.take = 0; seeds.m = (uint32_t) rnd.nextInt() | 1u;
    regenerate();
}
void UndertowProcessor::mutate()
{
    pushHistory(); edited = false;
    if (seeds.m == 0) seeds.m = (uint32_t) rnd.nextInt() | 1u;
    seeds.muts++;
    regenerate();
}
void UndertowProcessor::undo()
{
    if (history.empty()) return;
    future.push_back (snap()); restore (history.back()); history.pop_back();
}
void UndertowProcessor::redo()
{
    if (future.empty()) return;
    history.push_back (snap()); restore (future.back()); future.pop_back();
}

void UndertowProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buf.getNumSamples();
    buf.clear();
    midi.clear();

    double hostBpm = 128; bool hostPlaying = false; double ppq = 0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) hostBpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    bpm = (float) hostBpm;
    const bool internal = internalPlay.load() && ! hostPlaying;
    const bool playing = hostPlaying || internal;
    if (internal) { if (! wasPlaying) internalPpq = 0; ppq = internalPpq; }
    const double bps = hostBpm / 60.0 / sr;
    // if the DAW loops a region shorter than the pattern (FL pattern mode, a 1-bar cycle...), don't restart
    // the bassline every time: keep counting forward so every bar gets played
    if (hostPlaying)
    {
        if (! wasPlaying || lastHostEnd < 0) hostOffset = 0;
        else if (ppq < lastHostEnd - 0.05) hostOffset += lastHostEnd - ppq;   // host jumped back (loop)
        else if (ppq > lastHostEnd + 0.5) hostOffset = 0;                     // user jumped forward: follow
        lastHostEnd = ppq + n * bps;
        ppq += hostOffset;
    }
    else lastHostEnd = -1;
    if (playing && loopBeats > 0)
    {
        const auto cyc = (long long) std::floor (ppq / loopBeats);
        if (! wasPlaying) lastCycle = cyc;
        else if (cyc != lastCycle) { lastCycle = cyc; wrapped = true; }
    }

    if (! playing)
    {
        if (wasPlaying) { if (sounding >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, sounding), 0); voice.allOff(); sounding = -1; }
        wasPlaying = false; uiBeat = -1;
    }
    wasPlaying = playing;

    const int sound = (int) p ("sound"); const float tone = p ("tone"), drv = p ("drive"), glide = p ("glide");
    const float gain = juce::Decibels::decibelsToGain (p ("volume"));
    const bool kickOn = p ("kick") > 0.5f, muted = p ("mute") > 0.5f;

    // gather events for this block
    struct Hit { int off; const Ev* e; };
    juce::SpinLock::ScopedTryLockType lock (patLock);
    std::vector<Hit> hits; hits.reserve (32);
    std::vector<int> kicks;
    if (playing && lock.isLocked() && loopBeats > 0)
    {
        const double p0 = ppq, p1 = ppq + n * bps;
        for (auto& e : events)
        {
            double k = std::ceil ((p0 - e.beat) / loopBeats);
            for (double t = e.beat + k * loopBeats; t < p1; t += loopBeats)
                hits.push_back ({ juce::jlimit (0, n - 1, (int) ((t - p0) / bps)), &e });
        }
        for (double b = std::ceil (p0); b < p1; b += 1) kicks.push_back (juce::jlimit (0, n - 1, (int) ((b - p0) / bps)));
        std::stable_sort (hits.begin(), hits.end(), [] (const Hit& a, const Hit& b) { return a.off < b.off; });
        uiBeat = (float) std::fmod (p0, loopBeats);
    }

    auto* L = buf.getWritePointer (0); auto* R = buf.getNumChannels() > 1 ? buf.getWritePointer (1) : nullptr;
    size_t hi = 0, ki = 0; float peak = 0;
    for (int i = 0; i < n; ++i)
    {
        while (hi < hits.size() && hits[hi].off <= i)
        {
            const Ev& e = *hits[hi].e;
            if (e.on)
            {
                if (sounding >= 0 && sounding != e.note) midi.addEvent (juce::MidiMessage::noteOff (1, sounding), i);
                midi.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel), i);
                voice.noteOn (e.note, e.vel / 127.f, e.slide, glide);
                sounding = e.note; uiNoteIdx = e.idx;
            }
            else if (e.note == sounding)
            {
                midi.addEvent (juce::MidiMessage::noteOff (1, e.note), i);
                voice.noteOff (e.note); sounding = -1;
            }
            ++hi;
        }
        while (ki < kicks.size() && kicks[ki] <= i) { if (kickOn) kick.trig(); duck = 0.25f; ++ki; }
        duck += (1.f - duck) * 0.0009f * (44100.f / (float) sr);
        float b = muted ? 0.f : voice.render (sound, tone, drv) * gain;
        if (kickOn) b *= duck;
        const float k = kickOn ? kick.render() * 0.5f : 0.f;
        const float y = b + k;
        L[i] = y; if (R) R[i] = y;
        peak = std::max (peak, std::abs (b));
        if ((i & 1) == 0) { const int w = scopeW.load (std::memory_order_relaxed); scope[(size_t) w] = b; scopeW.store ((w + 1) & 1023, std::memory_order_relaxed); }
    }
    uiLevel = std::max (peak, uiLevel.load() * 0.9f);
    if (internal) internalPpq += n * bps;
}

// ---------------------------------------------------------------- MIDI export
juce::String UndertowProcessor::suggestedFileName() const
{
    return "Undertow_" + juce::String (styleName ((int) p ("style"))).replace ("-", "") + "_" + keyNames[(int) p ("key")].replace ("#", "s")
         + juce::String (scaleName (scale())).replace (" ", "").replace (".", "") + "_" + juce::String (juce::roundToInt (bpm.load())) + "bpm.mid";
}

bool UndertowProcessor::writeMidi (const juce::File& f) const
{
    const int tpq = 960; juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm.load())), 0);
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Undertow Bass"), 0);
    const float sw = swing(); const int root = rootMidi(), sc = scale();
    auto beatOf = [&] (int step) { return step * 0.25 + ((step % 2) ? (sw - 0.5) * 0.5 : 0.0); };
    auto pat = getPattern();
    for (size_t i = 0; i < pat.size(); ++i)
    {
        const auto& nn = pat[i]; const int m = juce::jlimit (0, 127, root + degToSemi (nn.deg, sc));
        const double on = beatOf (nn.step) * tpq;
        const float gk = 0.3f + 0.67f * p ("gate");
        double off = (beatOf (nn.step) + std::max (0.06, (beatOf (nn.step + nn.len) - beatOf (nn.step)) * gk - 0.01)) * tpq;
        if (i + 1 < pat.size() && pat[i + 1].slide && pat[i + 1].step == nn.step + nn.len) off = (beatOf (nn.step + nn.len) + 0.02) * tpq;
        seq.addEvent (juce::MidiMessage::noteOn (1, m, (juce::uint8) nn.vel), on);
        seq.addEvent (juce::MidiMessage::noteOff (1, m), off);
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (tpq); mf.addTrack (seq);
    f.deleteFile(); juce::FileOutputStream os (f);
    return os.openedOk() && mf.writeTo (os);
}

// ---------------------------------------------------------------- state
void UndertowProcessor::getStateInformation (juce::MemoryBlock& mb)
{
    auto st = apvts.copyState();
    st.setProperty ("seedR", (juce::int64) seeds.r, nullptr); st.setProperty ("seedP", (juce::int64) seeds.p, nullptr);
    st.setProperty ("seedM", (juce::int64) seeds.m, nullptr); st.setProperty ("muts", seeds.muts, nullptr); st.setProperty ("take", (juce::int64) seeds.take, nullptr);
    st.setProperty ("edited", edited, nullptr); st.setProperty ("preset", currentPreset, nullptr);
    st.removeChild (st.getChildWithName ("PATTERN"), nullptr);
    if (edited)
    {
        juce::ValueTree pt ("PATTERN");
        for (auto& n : getPattern())
        {
            juce::ValueTree nt ("N");
            nt.setProperty ("s", n.step, nullptr); nt.setProperty ("l", n.len, nullptr); nt.setProperty ("d", n.deg, nullptr);
            nt.setProperty ("v", n.vel, nullptr); nt.setProperty ("sl", n.slide, nullptr); nt.setProperty ("g", n.ghost, nullptr);
            pt.appendChild (nt, nullptr);
        }
        st.appendChild (pt, nullptr);
    }
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, mb);
}
void UndertowProcessor::setStateInformation (const void* d, int s)
{
    if (auto xml = getXmlFromBinary (d, s))
    {
        auto st = juce::ValueTree::fromXml (*xml);
        if (! st.isValid()) return;
        seeds.r = (uint32_t) (juce::int64) st.getProperty ("seedR", 11); seeds.p = (uint32_t) (juce::int64) st.getProperty ("seedP", 7);
        seeds.m = (uint32_t) (juce::int64) st.getProperty ("seedM", 0); seeds.muts = st.getProperty ("muts", 0); seeds.take = (uint32_t) (juce::int64) st.getProperty ("take", 0);
        currentPreset = st.getProperty ("preset", 0);
        auto pt = st.getChildWithName ("PATTERN");
        std::vector<ut::Note> notes;
        for (auto nt : pt)
        {
            ut::Note n; n.step = nt.getProperty ("s"); n.len = nt.getProperty ("l"); n.deg = nt.getProperty ("d");
            n.vel = nt.getProperty ("v"); n.slide = nt.getProperty ("sl"); n.ghost = nt.getProperty ("g"); notes.push_back (n);
        }
        const bool wasEdited = st.getProperty ("edited", false) && ! notes.empty();
        st.removeChild (pt, nullptr);
        applyingPreset = true; apvts.replaceState (st); applyingPreset = false;
        edited = false;
        regenerate();
        if (wasEdited) applyEdit (notes);
        dirty = false; renderDirty = false; presetModified = false;
        history.clear(); future.clear();
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new UndertowProcessor(); }
juce::AudioProcessorEditor* UndertowProcessor::createEditor() { return new UndertowEditor (*this); }
