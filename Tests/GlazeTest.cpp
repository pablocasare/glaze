// Offline test: renders synthetic "vocals" through Glaze and measures the output pitch.
#include "../Source/PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
using glaze::gainToDb;
#include <cstdio>

static float measurePitch (const float* x, int n, double sr)
{
    // plain autocorrelation peak picking (independent from the plugin's YIN)
    int minLag = (int) (sr / 1000), maxLag = (int) (sr / 60);
    std::vector<double> ac ((size_t) maxLag + 1);
    double best = -1; int bestLag = 0;
    double e0 = 0; for (int i = 0; i < n - maxLag; ++i) e0 += x[i] * x[i];
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double s = 0, e1 = 0;
        for (int i = 0; i < n - maxLag; ++i) { s += x[i] * x[i + lag]; e1 += x[i + lag] * x[i + lag]; }
        ac[(size_t) lag] = s / std::sqrt (e0 * e1 + 1e-12);
    }
    for (int lag = minLag + 1; lag < maxLag; ++lag)
        if (ac[(size_t) lag] > best && ac[(size_t) lag] >= ac[(size_t) lag - 1] && ac[(size_t) lag] >= ac[(size_t) lag + 1]) { best = ac[(size_t) lag]; bestLag = lag; }
    // take the first peak that's close to the best (avoid octave errors)
    for (int lag = minLag + 1; lag < maxLag; ++lag)
        if (ac[(size_t) lag] > 0.9 * best && ac[(size_t) lag] >= ac[(size_t) lag - 1] && ac[(size_t) lag] >= ac[(size_t) lag + 1]) { bestLag = lag; break; }
    double a = ac[(size_t) bestLag - 1], b = ac[(size_t) bestLag], c = ac[(size_t) bestLag + 1];
    double t = bestLag + 0.5 * (a - c) / (a - 2 * b + c);
    return (float) (sr / t);
}

static float cents (float f, float ref) { return 1200.0f * std::log2 (f / ref); }

// harmonic, vowel-ish source with optional vibrato and pitch glide
static void synth (std::vector<float>& out, double sr, float f0, float detuneCents, float vibCents, float glideCents)
{
    double ph = 0;
    for (size_t i = 0; i < out.size(); ++i)
    {
        const double t = (double) i / sr;
        const double prog = t / ((double) out.size() / sr);
        const double c = detuneCents + vibCents * std::sin (2 * M_PI * 5.5 * t) + glideCents * prog;
        const double f = f0 * std::pow (2.0, c / 1200.0);
        ph += 2 * M_PI * f / sr;
        double s = 0;
        for (int h = 1; h <= 20; ++h)
        {
            const double hf = h * f;
            // crude formants at 700 Hz and 1200 Hz
            const double amp = (1.0 / h) * (1.0 + 2.0 * std::exp (-std::pow ((hf - 700) / 200, 2)) + 1.5 * std::exp (-std::pow ((hf - 1200) / 250, 2)));
            if (hf < sr * 0.45) s += amp * std::sin (h * ph);
        }
        out[i] = (float) (0.15 * s);
    }
}

static GlazeProcessor* makeDry()
{
    auto* p = new GlazeProcessor();
    // isolate tuning: switch other modules off
    for (auto id : { "gateOn", "eqOn", "compOn", "dsOn", "drvOn", "dblOn", "chOn", "dlOn", "rvOn", "limOn" })
        p->apvts.getParameter (id)->setValueNotifyingHost (0.0f);
    return p;
}

static void setParam (GlazeProcessor& p, const char* id, float v)
{
    auto* rp = p.apvts.getParameter (id);
    rp->setValueNotifyingHost (rp->convertTo0to1 (v));
}

static std::vector<float> render (GlazeProcessor& p, const std::vector<float>& in, double sr, int& latency)
{
    const int block = 256;
    p.setRateAndBufferSizeDetails (sr, block);
    p.prepareToPlay (sr, block);
    latency = p.getLatencySamples();
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    for (size_t pos = 0; pos < in.size(); pos += block)
    {
        const int n = (int) std::min<size_t> (block, in.size() - pos);
        juce::AudioBuffer<float> sub (buf.getArrayOfWritePointers(), 2, 0, n);
        for (int i = 0; i < n; ++i) { sub.setSample (0, i, in[pos + (size_t) i]); sub.setSample (1, i, in[pos + (size_t) i]); }
        p.processBlock (sub, midi);
        for (int i = 0; i < n; ++i) out[pos + (size_t) i] = 0.5f * (sub.getSample (0, i) + sub.getSample (1, i));
    }
    return out;
}

static void writeWav (const char* path, const std::vector<float>& a, const std::vector<float>& b, double sr)
{
    juce::File f (juce::File::getCurrentWorkingDirectory().getChildFile (path));
    f.deleteFile();
    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
    juce::AudioBuffer<float> buf (2, (int) a.size());
    for (size_t i = 0; i < a.size(); ++i) { buf.setSample (0, (int) i, a[i]); buf.setSample (1, (int) i, b[i]); }
    w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    int failures = 0;
    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        struct Case { const char* name; float f0; float det; float expectCentsFromRef; int scale; int key; };
        // A3 = 220 Hz. Chromatic: +38c snaps to A3. Minor in C: B (246.9) isn't in C minor -> nearest Bb/C.
        const Case cases[] {
            { "A3 +38c chromatic",  220.0f, 38.0f,  0.0f, 0, 0 },
            { "A3 -45c chromatic",  220.0f, -45.0f, 0.0f, 0, 0 },
            { "E4 +30c chromatic",  329.63f, 30.0f, 0.0f, 0, 0 },
            { "G2 -35c chromatic",  98.0f, -35.0f,  0.0f, 0, 0 },
            { "B3 +20c in C minor -> C4", 246.94f, 20.0f, 100.0f, 2, 0 },
        };
        for (auto& c : cases)
        {
            std::unique_ptr<GlazeProcessor> p (makeDry());
            setParam (*p, "tuneSpeed", 0.0f);
            setParam (*p, "scale", (float) c.scale);
            setParam (*p, "key", (float) c.key);
            std::vector<float> in ((size_t) (sr * 1.5));
            synth (in, sr, c.f0, c.det, 0, 0);
            int lat = 0;
            auto out = render (*p, in, sr, lat);
            const int start = (int) (sr * 0.7), len = (int) (sr * 0.5);
            const float fin = measurePitch (in.data() + start, len, sr);
            const float fout = measurePitch (out.data() + start, len, sr);
            const float err = cents (fout, c.f0) - c.expectCentsFromRef;
            double rmsIn = 0, rmsOut = 0;
            for (int i = start; i < start + len; ++i) { rmsIn += in[(size_t) i] * in[(size_t) i]; rmsOut += out[(size_t) i] * out[(size_t) i]; }
            const float lvl = gainToDb ((float) std::sqrt (rmsOut / rmsIn));
            const bool ok = std::abs (err) < 8.0f && std::abs (lvl) < 2.0f;
            failures += ok ? 0 : 1;
            std::printf ("%s sr=%6.0f %-28s in %+6.1fc -> out %+6.1fc (err %+5.1fc) level %+5.2f dB  lat=%d  %s\n",
                         ok ? "PASS" : "FAIL", sr, c.name, cents (fin, c.f0), cents (fout, c.f0), err, lvl, lat, ok ? "" : "<<<");
            if (sr == 44100.0 && c.det == 38.0f) writeWav ("test_tune_a3.wav", in, out, sr);
        }

        // Tune OFF must be transparent (output == delayed input)
        {
            std::unique_ptr<GlazeProcessor> p (makeDry());
            setParam (*p, "tuneOn", 0.0f);
            std::vector<float> in ((size_t) (sr * 1.0));
            synth (in, sr, 180.0f, 13.0f, 40.0f, 0);
            int lat = 0;
            auto out = render (*p, in, sr, lat);
            double errE = 0, sigE = 0;
            for (size_t i = (size_t) sr / 4; i < in.size(); ++i) { const float d = out[i] - in[i - (size_t) lat]; errE += d * d; sigE += in[i - (size_t) lat] * in[i - (size_t) lat]; }
            const float snr = 10.0f * std::log10 ((float) (sigE / (errE + 1e-20)));
            const bool ok = snr > 40.0f;
            failures += ok ? 0 : 1;
            std::printf ("%s sr=%6.0f tune off transparency: %.1f dB SNR vs delayed input\n", ok ? "PASS" : "FAIL", sr, snr);
        }

        // Full chain (all default modules) with a vibrato glide: no NaNs, sane level
        {
            std::unique_ptr<GlazeProcessor> p (new GlazeProcessor());
            std::vector<float> in ((size_t) (sr * 3.0));
            synth (in, sr, 200.0f, 0, 30.0f, 400.0f);
            int lat = 0;
            auto out = render (*p, in, sr, lat);
            float pk = 0; bool finite = true;
            for (auto v : out) { pk = std::max (pk, std::abs (v)); finite &= std::isfinite (v); }
            const bool ok = finite && pk < 1.0f && pk > 0.05f;
            failures += ok ? 0 : 1;
            std::printf ("%s sr=%6.0f full chain: peak %.3f, finite=%d\n", ok ? "PASS" : "FAIL", sr, pk, (int) finite);
            if (sr == 44100.0)
            {
                std::vector<float> l (out.size()), r (out.size());
                // re-render stereo for listening
                writeWav ("test_full_chain.wav", out, out, sr);
            }
        }
    }
    // Presets load
    {
        GlazeProcessor p;
        for (int i = 0; i < p.getNumPrograms(); ++i) { p.setCurrentProgram (i); std::printf ("preset %d: %s\n", i, p.getProgramName (i).toRawUTF8()); }
    }
    std::printf (failures == 0 ? "\nALL TESTS PASSED\n" : "\n%d FAILURES\n", failures);
    return failures == 0 ? 0 : 1;
}
