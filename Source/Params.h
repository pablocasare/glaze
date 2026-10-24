#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace glaze
{
// ---- Choice lists ---------------------------------------------------------
inline const juce::StringArray keyNames   { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
inline const juce::StringArray scaleNames { "Chromatic", "Major", "Minor", "Harm. Minor", "Maj. Penta", "Min. Penta", "Dorian" };
inline const juce::StringArray divNames   { "1/2", "1/4", "1/4 D", "1/4 T", "1/8", "1/8 D", "1/8 T", "1/16" };
inline const float divBeats[] { 2.0f, 1.0f, 1.5f, 2.0f / 3.0f, 0.5f, 0.75f, 1.0f / 3.0f, 0.25f };

// Scale masks: bit n set => pitch class n (relative to key) is allowed.
inline int scaleMask (int scale)
{
    auto m = [] (std::initializer_list<int> pcs) { int r = 0; for (auto p : pcs) r |= 1 << p; return r; };
    switch (scale)
    {
        case 1:  return m ({ 0, 2, 4, 5, 7, 9, 11 });
        case 2:  return m ({ 0, 2, 3, 5, 7, 8, 10 });
        case 3:  return m ({ 0, 2, 3, 5, 7, 8, 11 });
        case 4:  return m ({ 0, 2, 4, 7, 9 });
        case 5:  return m ({ 0, 3, 5, 7, 10 });
        case 6:  return m ({ 0, 2, 3, 5, 7, 9, 10 });
        default: return 0xFFF;
    }
}

// ---- Parameter layout -----------------------------------------------------
inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto fl = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit, float skewMid = -1.0f, float step = 0.0f)
    {
        NormalisableRange<float> r (lo, hi, step);
        if (skewMid > 0) r.setSkewForCentre (skewMid);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                             AudioParameterFloatAttributes().withLabel (unit)));
    };
    auto bo = [&] (const char* id, const char* name, bool def)
    { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto ch = [&] (const char* id, const char* name, const StringArray& c, int def)
    { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def)); };

    // Global / home macros
    fl ("inGain",  "Input",  -24, 24, 0, "dB", -1, 0.1f);
    fl ("outGain", "Output", -24, 24, 0, "dB", -1, 0.1f);
    fl ("mix",     "Mix",      0, 100, 100, "%");
    fl ("punch",   "Punch",    0, 200, 100, "%");
    fl ("space",   "Space",    0, 200, 100, "%");
    fl ("width",   "Width",    0, 200, 100, "%");
    bo ("lowLatency", "Low Latency", false);

    // Tune
    bo ("tuneOn", "Tune On", true);
    fl ("tuneAmount", "Tune Amount", 0, 100, 100, "%");
    fl ("tuneSpeed",  "Retune Speed", 0, 200, 12, "ms", 40);
    ch ("key",   "Key",   keyNames, 0);
    ch ("scale", "Scale", scaleNames, 2);
    fl ("humanize",  "Humanize", 0, 100, 0, "%");
    fl ("formant",   "Formant", -6, 6, 0, "st", -1, 0.1f);
    fl ("transpose", "Transpose", -12, 12, 0, "st", -1, 1.0f);

    // Gate
    bo ("gateOn", "Gate On", false);
    fl ("gateThresh",  "Gate Threshold", -80, 0, -50, "dB");
    fl ("gateRange",   "Gate Range", 0, 80, 18, "dB");
    fl ("gateAttack",  "Gate Attack", 0.1f, 50, 1.5f, "ms", 5);
    fl ("gateRelease", "Gate Release", 5, 1000, 120, "ms", 120);

    // EQ
    bo ("eqOn", "EQ On", true);
    fl ("eqLowCut",   "Low Cut", 20, 600, 90, "Hz", 120);
    fl ("eqBody",     "Body", -12, 12, 0, "dB", -1, 0.1f);
    fl ("eqBodyFreq", "Body Freq", 80, 800, 250, "Hz", 250);
    fl ("eqPres",     "Presence", -12, 12, 2, "dB", -1, 0.1f);
    fl ("eqPresFreq", "Presence Freq", 800, 8000, 3500, "Hz", 3000);
    fl ("eqAir",      "Air", -12, 12, 3, "dB", -1, 0.1f);

    // Compressor
    bo ("compOn", "Comp On", true);
    fl ("compThresh",  "Comp Threshold", -60, 0, -22, "dB");
    fl ("compRatio",   "Ratio", 1, 20, 4, ":1", 4);
    fl ("compAttack",  "Comp Attack", 0.1f, 100, 5, "ms", 10);
    fl ("compRelease", "Comp Release", 10, 1000, 90, "ms", 120);
    fl ("compMakeup",  "Makeup", 0, 24, 4, "dB", -1, 0.1f);
    fl ("compMix",     "Comp Mix", 0, 100, 100, "%");

    // De-esser
    bo ("dsOn", "De-Ess On", true);
    fl ("dsFreq",   "De-Ess Freq", 2000, 12000, 5500, "Hz", 5500);
    fl ("dsThresh", "De-Ess Threshold", -60, 0, -30, "dB");
    fl ("dsRange",  "De-Ess Range", 0, 24, 10, "dB");

    // Drive
    bo ("drvOn", "Drive On", false);
    fl ("drvAmount", "Drive", 0, 100, 25, "%");
    fl ("drvTone",   "Drive Tone", -100, 100, 0, "");
    fl ("drvMix",    "Drive Mix", 0, 100, 50, "%");

    // Doubler
    bo ("dblOn", "Double On", true);
    fl ("dblMix",    "Double Mix", 0, 100, 25, "%");
    fl ("dblDetune", "Detune", 0, 30, 9, "ct");
    fl ("dblDelay",  "Double Delay", 5, 40, 18, "ms");

    // Chorus
    bo ("chOn", "Chorus On", false);
    fl ("chRate",  "Chorus Rate", 0.05f, 5, 0.8f, "Hz", 0.8f);
    fl ("chDepth", "Chorus Depth", 0, 100, 25, "%");
    fl ("chMix",   "Chorus Mix", 0, 100, 40, "%");

    // Delay
    bo ("dlOn", "Delay On", true);
    bo ("dlSync", "Delay Sync", true);
    ch ("dlDiv", "Delay Division", divNames, 1);
    fl ("dlTime",     "Delay Time", 10, 2000, 300, "ms", 300);
    fl ("dlFeedback", "Feedback", 0, 95, 30, "%");
    fl ("dlMix",      "Delay Mix", 0, 100, 12, "%");
    bo ("dlPingPong", "Ping Pong", true);
    fl ("dlLowCut",   "Delay Low Cut", 20, 2000, 400, "Hz", 300);
    fl ("dlHighCut",  "Delay High Cut", 1000, 20000, 6000, "Hz", 5000);

    // Reverb
    bo ("rvOn", "Verb On", true);
    fl ("rvSize",     "Verb Size", 0, 100, 65, "%");
    fl ("rvDamp",     "Verb Damp", 0, 100, 45, "%");
    fl ("rvPreDelay", "Pre-Delay", 0, 200, 25, "ms");
    fl ("rvLowCut",   "Verb Low Cut", 20, 1500, 300, "Hz", 300);
    fl ("rvWidth",    "Verb Width", 0, 100, 100, "%");
    fl ("rvMix",      "Verb Mix", 0, 100, 16, "%");

    // Limiter
    bo ("limOn", "Limit On", true);
    fl ("limGain",    "Limit Gain", 0, 24, 2, "dB", -1, 0.1f);
    fl ("limCeiling", "Ceiling", -12, 0, -1, "dB", -1, 0.1f);
    fl ("limRelease", "Limit Release", 10, 500, 80, "ms", 80);

    return { p.begin(), p.end() };
}

// ---- Factory presets ------------------------------------------------------
struct Preset
{
    const char* name;
    std::vector<std::pair<const char*, float>> values; // anything not listed uses defaults
};

inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> presets {
        { "Melodic", {} },
        { "Hard Tune", { { "tuneSpeed", 0 }, { "humanize", 0 }, { "dblMix", 35 }, { "dblDetune", 6 },
                         { "drvOn", 1 }, { "drvAmount", 20 }, { "drvMix", 35 }, { "rvMix", 12 },
                         { "dlDiv", 4 }, { "dlMix", 10 }, { "eqAir", 5 }, { "compThresh", -26 } } },
        { "Clean R&B", { { "tuneSpeed", 45 }, { "humanize", 55 }, { "chOn", 1 }, { "chMix", 18 },
                         { "rvSize", 75 }, { "rvMix", 24 }, { "dlDiv", 5 }, { "dlMix", 9 },
                         { "dblMix", 18 }, { "eqBody", 1.5f }, { "eqAir", 4 } } },
        { "Radio Lead", { { "tuneSpeed", 60 }, { "humanize", 30 }, { "compThresh", -30 }, { "compRatio", 6 },
                          { "compAttack", 2 }, { "compMakeup", 8 }, { "drvOn", 1 }, { "drvAmount", 30 },
                          { "drvMix", 40 }, { "rvMix", 8 }, { "dlMix", 6 }, { "limGain", 5 },
                          { "eqPres", 4 }, { "dblMix", 15 } } },
        { "Ethereal", { { "tuneSpeed", 20 }, { "rvSize", 95 }, { "rvMix", 42 }, { "rvPreDelay", 60 },
                        { "dlPingPong", 1 }, { "dlDiv", 1 }, { "dlFeedback", 55 }, { "dlMix", 24 },
                        { "chOn", 1 }, { "chMix", 30 }, { "chDepth", 40 }, { "dblMix", 40 }, { "eqAir", 7 } } },
        { "Lo-Fi Phone", { { "eqLowCut", 450 }, { "eqAir", -12 }, { "eqPres", 9 }, { "eqPresFreq", 1800 },
                           { "drvOn", 1 }, { "drvAmount", 65 }, { "drvTone", -40 }, { "drvMix", 70 },
                           { "dblOn", 0 }, { "rvMix", 10 }, { "rvSize", 30 }, { "dlMix", 0 } } },
        { "Dry Clean", { { "tuneSpeed", 30 }, { "humanize", 40 }, { "dblOn", 0 }, { "dlOn", 0 },
                         { "rvOn", 0 }, { "gateOn", 1 } } },
    };
    return presets;
}
} // namespace glaze
