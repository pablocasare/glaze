#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>

struct Head : juce::AudioPlayHead
{
    double ppq = 0;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (128); p.setIsPlaying (true); p.setPpqPosition (ppq); return p; }
};

static int failures = 0;
static void check (bool ok, const char* what, float v) { std::printf ("%s  %-58s %.2f\n", ok ? "PASS" : "FAIL", what, v); failures += ok ? 0 : 1; }
static void setP (LiftoffProcessor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
static float db (double ms) { return 10.0f * std::log10 ((float) ms + 1e-20f); }

int main (int, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000; const int block = 256;
    auto run = [&] (LiftoffProcessor& p, std::function<float (int)> src, int samples, std::vector<float>& outL)
    {
        juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m;
        for (int pos = 0; pos < samples; pos += block)
        {
            for (int i = 0; i < block; ++i) { const float x = src (pos + i); buf.setSample (0, i, x); buf.setSample (1, i, x); }
            p.processBlock (buf, m);
            for (int i = 0; i < block; ++i) outL.push_back (buf.getSample (0, i));
        }
    };
    auto sine = [&] (float f, float a) { return [=] (int n) { return a * (float) std::sin (2 * M_PI * f * n / sr); }; };
    auto ms = [] (const std::vector<float>& v, size_t a, size_t b) { double s = 0; for (size_t i = a; i < b; ++i) s += v[i] * v[i]; return s / (double) (b - a); };

    // 1) build 0 is transparent
    {
        LiftoffProcessor p; Head h; p.setPlayHead (&h); p.prepareToPlay (sr, block);
        std::vector<float> out; run (p, sine (1000, 0.5f), (int) sr, out);
        const float d = db (ms (out, 24000, 48000)) - db (0.125);
        check (std::abs (d) < 0.2f, "build 0%: transparent at 1 kHz (level change dB)", d);
        std::vector<float> out2; LiftoffProcessor p2; p2.setPlayHead (&h); p2.prepareToPlay (sr, block);
        run (p2, sine (80, 0.5f), (int) sr, out2);
        const float d2 = db (ms (out2, 24000, 48000)) - db (0.125);
        check (std::abs (d2) < 0.5f, "build 0%: keeps the low end (80 Hz level change dB)", d2);
    }
    // 2) build 100 kills lows, adds noise + tails
    {
        LiftoffProcessor p; Head h; p.setPlayHead (&h); setP (p, "build", 100); setP (p, "noiseOn", 0); p.prepareToPlay (sr, block);
        std::vector<float> out; run (p, sine (80, 0.5f), (int) sr, out);
        const float d = db (ms (out, 24000, 48000)) - db (0.125);
        check (d < -20.0f, "build 100%: low end removed (80 Hz, dB)", d);
        std::vector<float> o2; LiftoffProcessor p2; p2.setPlayHead (&h); setP (p2, "build", 100); p2.prepareToPlay (sr, block);
        run (p2, [] (int) { return 0.0f; }, (int) sr, o2);
        const float nz = db (ms (o2, 24000, 48000));
        check (nz > -45.0f && nz < -10.0f, "build 100%: noise riser audible on silence (dBFS)", nz);
        bool finite = true; float pk = 0; for (auto v : out) { finite &= std::isfinite (v); pk = std::max (pk, std::abs (v)); }
        check (finite && pk < 2.0f, "build 100%: stable, no blowup (peak)", pk);
    }
    // 3) the drop: build 100 -> 0, tails gone within ~100 ms
    {
        LiftoffProcessor p; Head h; p.setPlayHead (&h); setP (p, "build", 100); p.prepareToPlay (sr, block);
        std::vector<float> out;
        // drum-ish bursts during the build
        run (p, [&] (int n) { return (n % 12000) < 2000 ? 0.6f * (float) std::sin (2 * M_PI * 1500 * n / sr) : 0.0f; }, (int) sr * 2, out);
        setP (p, "build", 0);
        std::vector<float> after; run (p, [] (int) { return 0.0f; }, (int) (sr * 0.5), after);
        const float tail = db (ms (after, (size_t) (sr * 0.1), (size_t) (sr * 0.5)));
        check (tail < -80.0f, "drop: delay/reverb/noise tails cut after drop (dBFS)", tail);
    }

    // 3b) auto build: drop at bar 17 over 8 bars (4/4 at 128 bpm)
    {
        LiftoffProcessor p; Head h; p.setPlayHead (&h);
        setP (p, "autoOn", 1); setP (p, "dropBar", 17); setP (p, "autoLen", 3);
        p.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m;
        auto at = [&] (double bar) { h.ppq = (bar - 1.0) * 4.0; buf.clear(); p.processBlock (buf, m); return p.uiAuto.load(); };
        const float a5 = at (5.0), a9 = at (9.01), a13 = at (13.0), a169 = at (16.9), a171 = at (17.05);
        check (a5 == 0.0f && a9 < 0.01f, "auto: silent before bar 9", a5 + a9);
        check (std::abs (a13 - 0.5f) < 0.01f, "auto: 50% built at bar 13", a13);
        check (a169 > 0.97f, "auto: ~100% just before the drop (bar 16.9)", a169);
        check (a171 == 0.0f, "auto: drops to 0 on bar 17", a171);
    }

    // 4) UI frames: build up over 4 s, drop, aftermath
    const juce::File outDir (argv[1] ? argv[1] : ".");
    {
        LiftoffProcessor p; Head h; p.setPlayHead (&h); p.setRateAndBufferSizeDetails (sr, block); p.prepareToPlay (sr, block);
        std::unique_ptr<LiftoffEditor> ed (dynamic_cast<LiftoffEditor*> (p.createEditor()));
        juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer m;
        auto save = [&] (const juce::String& name)
        {
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = outDir.getChildFile (name); f.getParentDirectory().createDirectory(); f.deleteFile();
            juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
        };
        const int frames = 60 * 7;
        bool savedChill = false, savedMid = false, savedMax = false, savedDrop = false;
        int n = 0;
        for (int fr = 0; fr < frames; ++fr)
        {
            const float t = fr / 60.0f;
            const float build = t < 0.7f ? 0.0f : (t < 4.7f ? (t - 0.7f) / 4.0f * 100.0f : (t < 5.0f ? 100.0f : 0.0f));
            setP (p, "build", build);
            for (int b = 0; b < 3; ++b)
            {
                for (int i = 0; i < block; ++i, ++n) { const float x = (n % 11250) < 1500 ? 0.5f * (float) std::sin (n * 0.02) : 0.05f * (float) std::sin (n * 0.07); buf.setSample (0, i, x); buf.setSample (1, i, x); }
                p.processBlock (buf, m); h.ppq += block * 128.0 / 60.0 / sr;
            }
            ed->timerCallback();
            if (fr % 2 == 0) save ("anim/f" + juce::String (fr / 2).paddedLeft ('0', 4) + ".png");
            if (! savedChill && t > 0.5f) { save ("liftoff_chill.png"); savedChill = true; }
            if (! savedMid && build > 55) { save ("liftoff_building.png"); savedMid = true; }
            if (! savedMax && t > 4.85f) { save ("liftoff_max.png"); savedMax = true; }
            if (! savedDrop && ed->getDial().blasting() && t > 5.25f) { save ("liftoff_drop.png"); savedDrop = true; }
        }
        check (savedChill && savedMid && savedMax && savedDrop, "animation: chill / building / max / drop frames reached", 1);
    }
    std::printf (failures ? "\n%d FAILURES\n" : "\nALL TESTS PASSED\n", failures);
    return failures;
}
