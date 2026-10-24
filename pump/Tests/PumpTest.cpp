#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>

struct FakeHead : juce::AudioPlayHead
{
    double ppq = 0, bpm = 120; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setIsPlaying (playing); p.setPpqPosition (ppq); return p;
    }
};

static int failures = 0;
static void check (bool ok, const char* what, float val)
{
    std::printf ("%s  %-55s %.3f\n", ok ? "PASS" : "FAIL", what, val);
    failures += ok ? 0 : 1;
}
static void setP (PumpProcessor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }

int main (int, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000; const int block = 128;

    // ---------- tempo mode ----------
    {
        PumpProcessor p; FakeHead head; p.setPlayHead (&head);
        p.setRateAndBufferSizeDetails (sr, block); p.prepareToPlay (sr, block);
        setP (p, "depth", 100); setP (p, "length", 50); setP (p, "curve", 0); setP (p, "rate", 2); // 1/4 note, linear
        juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m;
        std::vector<float> gains;
        const double beatsPerBlock = block * 120.0 / 60.0 / sr;
        for (int b = 0; b < 1200; ++b)
        {
            for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) buf.setSample (c, i, 1.0f); // DC => output = gain
            p.processBlock (buf, m);
            for (int i = 0; i < block; ++i) gains.push_back (buf.getSample (0, i));
            head.ppq += beatsPerBlock;
        }
        const int perBeat = (int) (sr * 0.5); // 120 bpm
        auto at = [&] (double beatFrac) { return gains[(size_t) (perBeat * 2 + beatFrac * perBeat)]; }; // 3rd beat
        check (at (0.01) < 0.1f,  "tempo: fully ducked just after the beat", at (0.01));
        check (std::abs (at (0.25) - 0.5f) < 0.05f, "tempo: halfway back at 1/4 of beat (linear, len 50%)", at (0.25));
        check (at (0.75) > 0.98f, "tempo: fully recovered after length", at (0.75));
        float maxStep = 0; for (size_t i = 1; i < gains.size(); ++i) maxStep = std::max (maxStep, std::abs (gains[i] - gains[i - 1]));
        check (maxStep < 0.02f, "tempo: no clicks (max per-sample gain jump)", maxStep);
    }

    // ---------- sidechain mode ----------
    {
        PumpProcessor p; FakeHead head; head.playing = false; p.setPlayHead (&head);
        auto layout = p.getBusesLayout();
        layout.inputBuses.getReference (1) = juce::AudioChannelSet::stereo();
        check (p.setBusesLayout (layout), "sidechain: stereo key input accepted", 1);
        p.setRateAndBufferSizeDetails (sr, block); p.prepareToPlay (sr, block);
        setP (p, "mode", 1); setP (p, "depth", 100); setP (p, "length", 50); setP (p, "rate", 2); setP (p, "thresh", -20);
        juce::AudioBuffer<float> buf (4, block); juce::MidiBuffer m;
        std::vector<float> gains;
        const int kickAt = (int) (sr * 0.5);
        int n = 0;
        for (int b = 0; b < (int) (sr * 1.5 / block); ++b)
        {
            for (int i = 0; i < block; ++i, ++n)
            {
                buf.setSample (0, i, 1.0f); buf.setSample (1, i, 1.0f);
                const int k = n - kickAt;
                const float kick = (k >= 0 && k < 4000) ? 0.9f * std::exp (-k / 1200.0f) * std::sin (k * 0.0125f) : 0.0f;
                buf.setSample (2, i, kick); buf.setSample (3, i, kick);
            }
            p.processBlock (buf, m);
            for (int i = 0; i < block; ++i) gains.push_back (buf.getSample (0, i));
        }
        check (gains[(size_t) (kickAt - 100)] > 0.99f, "sidechain: no duck before the kick", gains[(size_t) (kickAt - 100)]);
        check (gains[(size_t) (kickAt + 480)] < 0.15f, "sidechain: ducked 10ms after the kick", gains[(size_t) (kickAt + 480)]);
        check (gains[(size_t) (kickAt + 24000)] > 0.98f, "sidechain: recovered after half a beat", gains[(size_t) (kickAt + 24000)]);
        check (std::abs (gains.back() - 1.0f) < 1e-3f, "sidechain: stays open with no further kicks", gains.back());
        int triggers = 0; for (size_t i = 1; i < gains.size(); ++i) if (gains[i] < 0.5f && gains[i - 1] >= 0.5f) ++triggers;
        check (triggers == 1, "sidechain: one kick = exactly one duck", (float) triggers);
    }

    // ---------- UI snapshots + animation frames ----------
    const juce::File out (argv[1] ? argv[1] : ".");
    for (int mode = 0; mode < 2; ++mode)
    {
        PumpProcessor p; FakeHead head; p.setPlayHead (&head);
        p.setRateAndBufferSizeDetails (sr, block); p.prepareToPlay (sr, block);
        setP (p, "mode", (float) mode);
        std::unique_ptr<PumpEditor> ed (dynamic_cast<PumpEditor*> (p.createEditor()));
        juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m;
        auto save = [&] (const juce::String& name)
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = out.getChildFile (name); f.getParentDirectory().createDirectory();
            f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        };
        const int framesTotal = mode == 0 ? 330 : 2;
        bool savedSlam = false, savedFat = false, savedPop = false;
        for (int fr = 0; fr < framesTotal; ++fr)
        {
            for (int b = 0; b < 6; ++b) // ~1/60 s of audio per UI frame
            {
                for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) buf.setSample (c, i, 0.5f);
                p.processBlock (buf, m);
                head.ppq += block * 2.0 / sr;
            }
            ed->timerCallback();
            auto& st = ed->getStage();
            if (mode == 0)
            {
                if (fr % 2 == 0) save ("anim/f" + juce::String (fr / 2).paddedLeft ('0', 4) + ".png");
                const double ph = std::fmod (head.ppq, 1.0);
                if (! savedSlam && st.strokes == 2 && ph > 0.06 && ph < 0.12) { save ("pump_slam.png"); savedSlam = true; }
                if (! savedFat && st.strokes == 6 && ph > 0.5) { save ("pump_inflated.png"); savedFat = true; }
                if (! savedPop && st.isPopping()) { for (int k = 0; k < 6; ++k) ed->timerCallback(); save ("pump_pop.png"); savedPop = true; }
            }
        }
        check (mode == 1 || (savedSlam && savedFat && savedPop), "animation: slam / inflate / pop frames reached", (float) mode);
        if (mode == 1) save ("pump_sidechain.png");
    }
    std::printf (failures ? "\n%d FAILURES\n" : "\nALL TESTS PASSED\n", failures);
    return failures;
}
