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
    const bool legato = gate && slide && ! arm;
    if (legato)
    {
        glideCoef = 1.f - std::exp (-1.f / (float) (std::max (5.f, glideMs) * 0.001f * sr));
        note = n; vel = v; gate = true;
        return;
    }
    pendingNote = n; pendingVel = v;
    // fade the previous note out before the new one speaks, so a retrigger never clicks
    if (amp > 0.025f) { arm = true; gate = false; }
    else startNow (n, v);
}

void Voice::startNow (int n, float v)
{
    freq = target; glideCoef = 1.f;
    pitchEnv = 1.f; fenv = 0.f; perc = 1.f;
    ic1 = ic2 = ic1R = ic2R = 0;
    note = n; vel = v; gate = true; arm = false; pendingNote = -1;
}

static float warmSat (float x, float amount)
{
    const float g = 1.f + amount * 4.2f;
    float y = x * g;
    y += 0.16f * amount * y * std::abs (y); // a little even-harmonic warmth, not fizz
    return std::tanh (y) / std::tanh (g * 0.72f);
}
static float svfLow (float x, float g, float k, float& ic1, float& ic2)
{
    const float a1 = 1.f / (1.f + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
    if (std::abs (ic1) < 1e-15f) ic1 = 0; if (std::abs (ic2) < 1e-15f) ic2 = 0;
    return v2;
}

void Voice::render (int sound, float cutOff, float drive, float& left, float& right)
{
    // sub, saw, square, detune, cutoff Hz, resonance, filter env (oct), env decay, drive, pitch drop (semitones),
    // attack, release, output trim. Reese and Growl get a little stereo above the mono sub.
    struct R { float sub, saw, sq, det, cut, res, env, dec, drv, pe, atk, rel, trim; int wide; };
    static const R rec[] = {
        { 1.00f, 0.00f, 0.00f, 0.00f,  170, 0.10f, 0.55f, 0.09f, 0.18f, 0.0f, 0.0040f, 0.050f, 0.90f, 0 }, // Sub
        { 0.82f, 0.00f, 0.38f, 0.00f,  340, 0.50f, 3.30f, 0.075f, 0.32f, 7.0f, 0.0012f, 0.020f, 0.78f, 0 }, // Rubber
        { 0.62f, 0.52f, 0.00f, 0.018f, 460, 0.22f, 1.15f, 0.26f, 0.38f, 0.0f, 0.0060f, 0.055f, 0.62f, 1 }, // Reese
        { 0.95f, 0.00f, 0.00f, 0.00f, 1400, 0.06f, 0.25f, 0.14f, 0.16f, 0.0f, 0.0030f, 0.070f, 0.70f, 0 }, // Organ
        { 0.28f, 0.82f, 0.00f, 0.00f,  190, 0.74f, 4.60f, 0.15f, 0.42f, 0.0f, 0.0018f, 0.026f, 0.58f, 0 }, // Acid
        { 0.55f, 0.42f, 0.32f, 0.010f, 400, 0.46f, 2.10f, 0.16f, 0.55f, 2.2f, 0.0035f, 0.034f, 0.55f, 1 } }; // Growl
    const R& r = rec[juce::jlimit (0, 5, sound)];

    if (arm)
    {
        amp *= std::exp (-1.f / (0.0004f * (float) sr));
        if (amp < 0.02f && pendingNote >= 0) startNow (pendingNote, pendingVel);
    }

    const float atk = gate ? (1.f - std::exp (-1.f / (r.atk * (float) sr))) : (1.f - std::exp (-1.f / (r.rel * (float) sr)));
    amp += ((gate ? 1.f : 0.f) - amp) * atk;
    if (! gate && amp < 1.0e-5f && ! arm)
    {
        amp = 0; ic1 = ic2 = ic1R = ic2R = 0; left = right = 0; return;
    }

    if (glideCoef < 0.999f)
    {
        const float st = 12.f * std::log2 (std::max (20.f, freq) / 440.f);
        const float stT = 12.f * std::log2 (std::max (20.f, target) / 440.f);
        freq = 440.f * std::pow (2.f, (st + (stT - st) * glideCoef) / 12.f);
    }
    else freq = target;
    pitchEnv *= std::exp (-1.f / (0.032f * (float) sr));
    fenv += ((gate && fenv < 0.98f ? 1.f : 0.f) - fenv) * (fenv < 0.98f && gate ? (1.f - std::exp (-1.f / (0.0025f * (float) sr))) : 0.f);
    if (! (gate && fenv < 0.98f)) fenv *= std::exp (-1.f / (r.dec * (float) sr));
    perc *= std::exp (-1.f / (0.045f * (float) sr));
    lfo += 0.18 / sr; if (lfo >= 1) lfo -= 1;
    growl += (sound == 5 ? 31.0 : 0.0) / sr; if (growl >= 1) growl -= 1;

    const double play = freq * (r.pe > 0 ? std::pow (2.0, r.pe * (double) pitchEnv / 12.0) : 1.0);
    const double os = sr * 2.0;
    const double dt = play / os;
    const double drift = 0.35 * std::sin (juce::MathConstants<double>::twoPi * lfo);
    const float wob = sound == 5 ? (1.f + 0.30f * (float) std::sin (juce::MathConstants<double>::twoPi * growl)) : 1.f;
    float fc = r.cut * wob * std::pow (2.f, cutOff * 2.4f + r.env * fenv * (0.45f + 0.55f * vel));
    fc = juce::jlimit (28.f, (float) sr * 0.36f, fc);
    const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) os);
    const float k = 2.f - 1.82f * r.res;
    const float satAmt = juce::jlimit (0.f, 1.6f, r.drv + drive * 0.85f);

    float accL = 0, accR = 0;
    for (int step = 0; step < 2; ++step)
    {
        auto wrap = [] (double& p, double inc) { p += inc; while (p >= 1.0) p -= 1.0; while (p < 0) p += 1.0; };
        wrap (subPhase, dt);
        wrap (phase, dt);
        const double d1 = 1.0 + (double) r.det * (1.15 + drift);
        const double d2 = 1.0 - (double) r.det * (0.92 + drift * 0.8);
        wrap (phase2, dt * d1);
        wrap (phase3, dt * d2);

        const float sub = (float) std::sin (juce::MathConstants<double>::twoPi * subPhase);
        float mono = r.sub * sub;
        if (sound == 0) mono += 0.07f * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 2.0); // a little warmth, still a sub
        if (sound == 3)
        {
            mono += 0.40f * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 2.0);
            mono += 0.18f * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 3.0);
            mono += 0.07f * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 4.0);
            mono += 0.22f * perc * (float) std::sin (juce::MathConstants<double>::twoPi * subPhase * 2.0); // key click, short and dark
        }
        auto sawAt = [&] (double p, double inc) { return (float) (2.0 * p - 1.0) - polyBlep (p, inc); };
        auto squareAt = [&] (double p, double inc) {
            float q = p < 0.5 ? 1.f : -1.f;
            q += polyBlep (p, inc);
            double p2 = p + 0.5; if (p2 >= 1.0) p2 -= 1.0;
            q -= polyBlep (p2, inc);
            return q;
        };
        float side = 0;
        if (r.saw > 0)
        {
            const float s1 = sawAt (phase, dt);
            if (r.wide)
            {
                const float s2 = sawAt (phase2, dt * d1), s3 = sawAt (phase3, dt * d2);
                mono += r.saw * (0.40f * s1 + 0.32f * s2 + 0.28f * s3);
                side = 0.20f * (s2 - s3);
            }
            else mono += r.saw * s1;
        }
        if (r.sq > 0) mono += r.sq * squareAt (phase, dt);

        const float m = warmSat (svfLow (mono, g, k, ic1, ic2), satAmt);
        const float sd = r.wide ? warmSat (svfLow (side, g, k, ic1R, ic2R), satAmt * 0.5f) : 0.f;
        accL += m + sd; accR += m - sd;
    }

    const float gain = r.trim * amp * (0.62f + 0.38f * vel);
    auto dc = [] (float x, float& px, float& py) {
        const float y = x - px + 0.995f * py; px = x; py = y; return y;
    };
    left = dc (accL * 0.5f * gain, dcX, dcY);
    right = dc (accR * 0.5f * gain, dcXR, dcYR);
}

float Kick::render()
{
    if (t > 0.40) return 0.f;
    const double f = tune + tune * 2.6 * std::exp (-t / 0.030);
    ph += f / sr;
    float y = (float) std::sin (juce::MathConstants<double>::twoPi * ph) * (float) std::exp (-t / 0.20);
    y += (t < 0.0025 ? 0.28f * (float) (1.0 - t / 0.0025) : 0.f);
    t += 1.0 / sr;
    return std::tanh (1.5f * y) * 0.62f;
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
    v.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (0.5f, 0.75f), 0.53f,
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
    v.push_back (std::make_unique<AudioParameterBool> (ParameterID { "keyIn", 1 }, "MIDI Key In", true));
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
void UndertowProcessor::parameterChanged (const juce::String& id, float value)
{
    if (! applyingPreset && ! applyingFeel) presetModified = true;
    if (id == "swing" && ! applyingFeel && ! applyingPreset) swingTouched = true;
    if (id == "style" && ! applyingPreset) { swingTouched = false; pendingSwingStyle = (int) value; }
    if (id == "key" || id == "scale" || id == "octave" || id == "swing" || id == "gate") renderDirty = true;
    else dirty = true;
}

const std::vector<UndertowProcessor::Preset>& UndertowProcessor::presets()
{
    using namespace ut;
    static const std::vector<Preset> P {
        { "Night Roller",       Rolling,     Minor,       ChordAuto,         .62f, .35f, .18f, .78f, .53f,  55, .28f, .50f, 1,  .00f, .18f },
        { "Warehouse Driver",   Driving,     Minor,       ChordStay,         .82f, .28f, .15f, .62f, .51f,  35, .18f, .62f, 5,  .05f, .38f },
        { "Offbeat Classic",    Offbeat,     Dorian,      ChordI_VII,        .42f, .30f, .12f, .88f, .54f,  50, .12f, .42f, 3, -.05f, .12f },
        { "Bouncy Tech",        Bouncy,      Minor,       ChordI_VI,         .55f, .48f, .28f, .48f, .54f,  45, .32f, .58f, 1,  .12f, .24f },
        { "Garage Swing",       TwoStep,     Minor,       ChordI_VI_VII,     .58f, .42f, .22f, .92f, .60f,  95, .22f, .48f, 2, -.05f, .28f },
        { "Minimal Hypnosis",   Minimal,     Phrygian,    ChordStay,         .38f, .18f, .08f, .72f, .53f,  40, .18f, .36f, 0, -.12f, .08f },
        { "Off The Grid",       Broken,      Minor,       ChordI_iv,         .58f, .50f, .35f, .64f, .54f,  55, .30f, .52f, 1,  .08f, .26f },
        { "Squelch 303",        Acid,        Phrygian,    ChordStay,         .68f, .55f, .32f, .70f, .54f,  65, .15f, .72f, 4,  .15f, .40f },
        { "Speed Wobble",       SpeedGarage, Minor,       ChordI_VII,        .55f, .40f, .18f, .90f, .59f,  85, .18f, .50f, 2,  .00f, .42f },
        { "Tresillo Sun",       Afro,        Dorian,      ChordI_iv,         .52f, .40f, .20f, .78f, .55f,  48, .30f, .48f, 0,  .02f, .14f },
        { "Deep Undertow",      DeepTech,    Minor,       ChordI_VI,         .42f, .22f, .10f, .96f, .54f, 110, .16f, .38f, 0, -.18f, .16f },
        { "Bassline Bounce",    Bassline,    Minor,       ChordI_iv_VI_V,    .60f, .35f, .18f, .52f, .55f,  48, .20f, .60f, 5,  .12f, .40f },
        { "Late Night Reese",   Rolling,     HarmMinor,   ChordI_VI_VII,     .55f, .48f, .25f, .90f, .53f, 100, .22f, .46f, 2, -.05f, .30f },
        { "Sub Pressure",       Offbeat,     Minor,       ChordStay,         .40f, .15f, .08f, .92f, .53f,  35, .08f, .36f, 0, -.22f, .10f },
        { "Jackin Organ",       Bouncy,      Mixolydian,  ChordI_VII,        .58f, .42f, .22f, .58f, .55f,  48, .28f, .55f, 3,  .08f, .18f },
        { "Tunnel Vision",      Driving,     Phrygian,    ChordI_VI,         .90f, .32f, .20f, .48f, .51f,  32, .30f, .68f, 4, -.08f, .48f },
        { "Penthouse",          Rolling,     MinorPent,   ChordStay,         .55f, .22f, .10f, .84f, .53f,  50, .20f, .44f, 0, -.12f, .12f },
        { "Disco Organ",        Offbeat,     Mixolydian,  ChordI_VII,        .46f, .38f, .15f, .90f, .55f,  42, .12f, .40f, 3,  .04f, .14f } };
    return P;
}

void UndertowProcessor::applyPreset (int index)
{
    const auto& L = presets(); if (L.empty()) return;
    index = (index % (int) L.size() + (int) L.size()) % (int) L.size();
    const auto& P = L[(size_t) index];
    pushHistory();
    pendingSwingStyle = -1;
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

static juce::ValueTree notesToTree (const std::vector<ut::Note>& v, const char* name)
{
    juce::ValueTree pt (name);
    for (auto& n : v)
    {
        juce::ValueTree nt ("N");
        nt.setProperty ("s", n.step, nullptr); nt.setProperty ("l", n.len, nullptr); nt.setProperty ("d", n.deg, nullptr);
        nt.setProperty ("v", n.vel, nullptr); nt.setProperty ("sl", n.slide, nullptr); nt.setProperty ("g", n.ghost, nullptr);
        pt.appendChild (nt, nullptr);
    }
    return pt;
}
static std::vector<ut::Note> treeToNotes (const juce::ValueTree& pt)
{
    std::vector<ut::Note> notes;
    for (auto nt : pt)
    {
        ut::Note n; n.step = nt.getProperty ("s"); n.len = nt.getProperty ("l"); n.deg = nt.getProperty ("d");
        n.vel = nt.getProperty ("v"); n.slide = nt.getProperty ("sl"); n.ghost = nt.getProperty ("g"); notes.push_back (n);
    }
    return notes;
}
static bool slotParam (const juce::String& id) { return id != "evolve" && id != "kick" && id != "mute" && id != "volume" && id != "lockRhythm" && id != "lockPitch" && id != "keyIn"; }
void UndertowProcessor::storeSlot (int i)
{
    auto& s = slots[(size_t) i]; s.used = true; s.seeds = seeds; s.pat = getPattern(); s.edited = edited; s.params.clear();
    for (auto* prm : getParameters()) if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm)) if (slotParam (r->getParameterID())) s.params.push_back ({ r->getParameterID(), r->getValue() });
    currentSlot = i;
}
void UndertowProcessor::recallSlot (int i)
{
    const auto& s = slots[(size_t) i]; if (! s.used) return;
    pushHistory();
    pendingSwingStyle = -1;
    applyingPreset = true;
    for (auto& [id, v] : s.params) if (auto* q = apvts.getParameter (id)) { q->beginChangeGesture(); q->setValueNotifyingHost (v); q->endChangeGesture(); }
    applyingPreset = false;
    dirty = false; renderDirty = false;
    seeds = s.seeds; edited = s.edited;
    { const juce::SpinLock::ScopedLockType l (patLock); pattern = s.pat; rebuildEvents(); }
    patternVersion++; currentSlot = i;
}
void UndertowProcessor::clearSlot (int i) { slots[(size_t) i] = {}; if (currentSlot == i) currentSlot = -1; }

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
    const float sw = swing(); const int root = rootMidi(), sc = scale(), style = (int) p ("style");
    const float gate = p ("gate");
    loopBeats = bars() * 4.0;
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        const auto& n = pattern[i];
        const int midi = juce::jlimit (0, 127, root + degToSemi (n.deg, sc));
        const bool legatoNext = i + 1 < pattern.size() && pattern[i + 1].slide && pattern[i + 1].step == n.step + n.len;
        const double on = noteOnBeat (n.step, sw, style);
        double off = noteOffBeat (n.step, n.len, sw, gate, style, legatoNext);
        if (off < on + 0.02) off = on + 0.02;
        events.push_back ({ on, true, midi, n.vel, n.slide, (int) i });
        const double wrapped = std::fmod (off, loopBeats);
        events.push_back ({ wrapped < 0 ? wrapped + loopBeats : wrapped, false, midi, 0, false, (int) i });
    }
    std::sort (events.begin(), events.end(), [] (const Ev& a, const Ev& b) { return a.beat < b.beat || (a.beat == b.beat && ! a.on && b.on); });
}

void UndertowProcessor::timerCallback()
{
    // EVOLVE: every time the pattern comes around, play a new take on it (same hook, new variations + fills)
    const int pk = pendingKey.exchange (-1);
    if (pk >= 0 && pk != (int) p ("key")) if (auto* q = apvts.getParameter ("key")) { q->beginChangeGesture(); q->setValueNotifyingHost (q->convertTo0to1 ((float) pk)); q->endChangeGesture(); }
    const int swingStyle = pendingSwingStyle.exchange (-1);
    if (swingStyle >= 0 && ! swingTouched.load())
    {
        applyingFeel = true;
        if (auto* q = apvts.getParameter ("swing"))
            q->setValueNotifyingHost (q->convertTo0to1 (styleSwing (juce::jlimit (0, NumStyles - 1, swingStyle))));
        applyingFeel = false;
    }
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
    for (const auto md : midi)
    {
        const auto msg = md.getMessage();
        if (msg.isAllNotesOff() || msg.isAllSoundOff()) heldNotes.fill (0);
        else if (msg.isNoteOn()) heldNotes[(size_t) msg.getNoteNumber()] = 1;
        else if (msg.isNoteOff()) heldNotes[(size_t) msg.getNoteNumber()] = 0;
    }
    if (p ("keyIn") > 0.5f)
    {
        int low = -1;
        for (int i = 0; i < 128; ++i) if (heldNotes[(size_t) i]) { low = i; break; }
        if (low >= 0) pendingKey = low % 12;
    }
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
    if (hostPlaying) internalPlay = false; // the DAW took over: don't start playing again by itself when it stops
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
        if (wasPlaying) { if (sounding >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, sounding), 0); voice.allOff(); sounding = -1; soundingIdx = -1; preclosedIdx = -1; }
        wasPlaying = false; uiBeat = -1;
    }
    wasPlaying = playing;

    const int sound = (int) p ("sound"); const float tone = p ("tone"), drv = p ("drive"), glide = p ("glide");
    const float gain = juce::Decibels::decibelsToGain (p ("volume"));
    const bool kickOn = p ("kick") > 0.5f, muted = p ("mute") > 0.5f;

    // gather events for this block. Fixed arrays: this runs on the audio thread.
    struct Hit { int off; const Ev* e; };
    Hit hits[160]; int nHits = 0;
    int kickAt[48]; int nKicks = 0;
    juce::SpinLock::ScopedTryLockType lock (patLock);
    const int styleNow = (int) p ("style");
    if (playing && lock.isLocked() && loopBeats > 0)
    {
        const double p0 = ppq, p1 = ppq + n * bps;
        for (auto& e : events)
        {
            double k = std::ceil ((p0 - e.beat) / loopBeats);
            for (double t = e.beat + k * loopBeats; t < p1 && nHits < 160; t += loopBeats)
                hits[nHits++] = { juce::jlimit (0, n - 1, (int) ((t - p0) / bps)), &e };
        }
        if (p1 > p0)
            for (double s = std::ceil (p0 * 4.0); s < p1 * 4.0 - 1.0e-9 && nKicks < 48; s += 1.0)
            {
                const double beat = s * 0.25;
                if (beat < p0 - 1.0e-6 || beat >= p1) continue;
                const int si = (int) std::llround (s);
                if (si >= 0 && isKickStep (styleNow, si))
                    kickAt[nKicks++] = juce::jlimit (0, n - 1, (int) ((beat - p0) / bps));
            }
        std::stable_sort (hits, hits + nHits, [] (const Hit& a, const Hit& b) { return a.off < b.off; });
        uiBeat = (float) std::fmod (p0 < 0 ? p0 + loopBeats * std::ceil (-p0 / loopBeats) : p0, loopBeats);
    }

    auto* Lch = buf.getWritePointer (0); auto* Rch = buf.getNumChannels() > 1 ? buf.getWritePointer (1) : nullptr;
    int hi = 0, ki = 0; float peak = 0;
    float kickHz = 49.f;
    if (kickOn)
    {
        kickHz = 440.f * std::pow (2.f, (rootMidi() - 69) / 12.f);
        while (kickHz < 45.f) kickHz *= 2.f;
        while (kickHz > 64.f) kickHz *= 0.5f;
    }
    for (int i = 0; i < n; ++i)
    {
        while (hi < nHits && hits[hi].off <= i)
        {
            const Ev& e = *hits[hi].e;
            if (e.on)
            {
                // a slide overlaps the note it leaves, so a mono synth downstream actually glides
                if (sounding >= 0 && ! e.slide) { midi.addEvent (juce::MidiMessage::noteOff (1, sounding), i); preclosedIdx = soundingIdx; }
                midi.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) e.vel), i);
                voice.noteOn (e.note, e.vel / 127.f, e.slide, glide);
                sounding = e.note; soundingIdx = e.idx; uiNoteIdx = e.idx;
            }
            else
            {
                if (e.idx != preclosedIdx) midi.addEvent (juce::MidiMessage::noteOff (1, e.note), i);
                if (e.idx == preclosedIdx) preclosedIdx = -1;
                if (e.idx == soundingIdx) { voice.noteOff (e.note); sounding = -1; soundingIdx = -1; }
            }
            ++hi;
        }
        while (ki < nKicks && kickAt[ki] <= i) { if (kickOn) { kick.trig (kickHz); duck = 0.22f; } ++ki; }
        duck += (1.f - duck) * 0.00085f * (44100.f / (float) std::max (1.0, sr));
        float ls = 0, rs = 0;
        voice.render (sound, tone, drv, ls, rs); // keep the voice moving while muted, so unmute doesn't click
        if (muted) ls = rs = 0;
        else { ls *= gain; rs *= gain; }
        if (kickOn) { ls *= duck; rs *= duck; }
        const float k = kickOn ? kick.render() * 0.42f : 0.f;
        Lch[i] = ls + k; if (Rch) Rch[i] = rs + k;
        peak = std::max (peak, std::abs (ls));
        if ((i & 1) == 0) { const int w = scopeW.load (std::memory_order_relaxed); scope[(size_t) w] = ls; scopeW.store ((w + 1) & 1023, std::memory_order_relaxed); }
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

int UndertowProcessor::printTakes() const
{
    if (p ("evolve") < 0.5f || edited || bars() < 2) return 1;
    return juce::jlimit (1, 4, 32 / bars());
}

bool UndertowProcessor::writeMidi (const juce::File& f, int takes) const
{
    const int tpq = 960; juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm.load())), 0);
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "Undertow Bass"), 0);
    const float sw = swing(); const int root = rootMidi(), sc = scale(), style = (int) p ("style");
    const float gateAmt = p ("gate");
    // takes > 1: the current take, then the next EVOLVE takes after it, back to back
    auto pat = getPattern();
    if (takes > 1)
    {
        auto P = currentParams();
        for (int t = 1; t < takes; ++t)
        {
            P.take = seeds.take + (uint32_t) t;
            for (auto nn : generate (P)) { nn.step += t * bars() * 16; pat.push_back (nn); }
        }
    }
    for (size_t i = 0; i < pat.size(); ++i)
    {
        const auto& nn = pat[i]; const int m = juce::jlimit (0, 127, root + degToSemi (nn.deg, sc));
        const bool legatoNext = i + 1 < pat.size() && pat[i + 1].slide && pat[i + 1].step == nn.step + nn.len;
        const double onBeat = noteOnBeat (nn.step, sw, style);
        double offBeat = noteOffBeat (nn.step, nn.len, sw, gateAmt, style, legatoNext);
        if (offBeat < onBeat + 0.02) offBeat = onBeat + 0.02;
        const double on = onBeat * tpq, off = offBeat * tpq;
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
    st.removeChild (st.getChildWithName ("SLOTS"), nullptr);
    juce::ValueTree sl ("SLOTS"); sl.setProperty ("current", currentSlot, nullptr);
    for (int i = 0; i < NumSlots; ++i)
    {
        const auto& s = slots[(size_t) i]; if (! s.used) continue;
        auto t = notesToTree (s.pat, "SLOT"); t.setProperty ("i", i, nullptr);
        t.setProperty ("r", (juce::int64) s.seeds.r, nullptr); t.setProperty ("p", (juce::int64) s.seeds.p, nullptr); t.setProperty ("m", (juce::int64) s.seeds.m, nullptr);
        t.setProperty ("mu", s.seeds.muts, nullptr); t.setProperty ("tk", (juce::int64) s.seeds.take, nullptr); t.setProperty ("ed", s.edited, nullptr);
        juce::ValueTree ps ("P"); for (auto& [id, v] : s.params) ps.setProperty (id, v, nullptr);
        t.setProperty ("params", ps.toXmlString(), nullptr);
        sl.appendChild (t, nullptr);
    }
    st.appendChild (sl, nullptr);
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
        auto sl = st.getChildWithName ("SLOTS");
        for (auto& x : slots) x = {};
        for (auto t : sl)
        {
            const int i = t.getProperty ("i", -1); if (i < 0 || i >= NumSlots) continue;
            auto& x = slots[(size_t) i]; x.used = true; x.pat = treeToNotes (t);
            x.seeds.r = (uint32_t) (juce::int64) t.getProperty ("r"); x.seeds.p = (uint32_t) (juce::int64) t.getProperty ("p"); x.seeds.m = (uint32_t) (juce::int64) t.getProperty ("m");
            x.seeds.muts = t.getProperty ("mu"); x.seeds.take = (uint32_t) (juce::int64) t.getProperty ("tk"); x.edited = t.getProperty ("ed");
            if (auto px = juce::parseXML (t.getProperty ("params").toString())) { auto ps = juce::ValueTree::fromXml (*px); for (int k = 0; k < ps.getNumProperties(); ++k) x.params.push_back ({ ps.getPropertyName (k).toString(), (float) ps.getProperty (ps.getPropertyName (k)) }); }
        }
        currentSlot = sl.getProperty ("current", -1);
        st.removeChild (sl, nullptr);
        pendingSwingStyle = -1;
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
