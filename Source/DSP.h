#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>
#include <vector>
#include <array>
#include <atomic>
#include <complex>
#include "Params.h"

namespace glaze
{
static constexpr float kPi = 3.14159265358979f;

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)  { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float msCoeff (float ms, double sr) // one-pole smoothing coefficient
{
    if (ms <= 0.0f) return 0.0f;
    return std::exp (-1.0f / (float) (ms * 0.001 * sr));
}

// ===========================================================================
// RBJ biquad, no allocation, with magnitude response for the UI.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    enum Type { LowPass, HighPass, Peak, HighShelf, LowShelf, BandPass };

    void set (Type t, double sr, float f, float q, float gainDb = 0.0f)
    {
        f = juce::jlimit (5.0f, (float) (sr * 0.49), f);
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        const double cw = std::cos (w), sw = std::sin (w);
        const double alpha = sw / (2.0 * q);
        const double A = std::pow (10.0, gainDb / 40.0);
        double B0 = 1, B1 = 0, B2 = 0, A0 = 1, A1 = 0, A2 = 0;
        switch (t)
        {
            case LowPass:  B0 = (1 - cw) / 2; B1 = 1 - cw; B2 = (1 - cw) / 2; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case HighPass: B0 = (1 + cw) / 2; B1 = -(1 + cw); B2 = (1 + cw) / 2; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case BandPass: B0 = alpha; B1 = 0; B2 = -alpha; A0 = 1 + alpha; A1 = -2 * cw; A2 = 1 - alpha; break;
            case Peak:     B0 = 1 + alpha * A; B1 = -2 * cw; B2 = 1 - alpha * A; A0 = 1 + alpha / A; A1 = -2 * cw; A2 = 1 - alpha / A; break;
            case HighShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) + (A - 1) * cw + s); B1 = -2 * A * ((A - 1) + (A + 1) * cw); B2 = A * ((A + 1) + (A - 1) * cw - s);
                A0 = (A + 1) - (A - 1) * cw + s; A1 = 2 * ((A - 1) - (A + 1) * cw); A2 = (A + 1) - (A - 1) * cw - s; break;
            }
            case LowShelf:
            {
                const double s = 2 * std::sqrt (A) * alpha;
                B0 = A * ((A + 1) - (A - 1) * cw + s); B1 = 2 * A * ((A - 1) - (A + 1) * cw); B2 = A * ((A + 1) - (A - 1) * cw - s);
                A0 = (A + 1) + (A - 1) * cw + s; A1 = -2 * ((A - 1) + (A + 1) * cw); A2 = (A + 1) + (A - 1) * cw - s; break;
            }
        }
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
        a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }

    inline float process (float x) noexcept
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0; }

    float magnitude (double f, double sr) const
    {
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sr;
        std::complex<double> z1c = std::polar (1.0, -w), z2c = std::polar (1.0, -2.0 * w);
        auto num = (double) b0 + (double) b1 * z1c + (double) b2 * z2c;
        auto den = 1.0 + (double) a1 * z1c + (double) a2 * z2c;
        return (float) std::abs (num / den);
    }
};

// ===========================================================================
// Simple fractional delay line (power-of-two ring buffer, cubic interpolation).
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;

    void prepare (int maxSamples)
    {
        int n = 1;
        while (n < maxSamples + 4) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1; w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    inline void push (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // delay in samples, >= 1 (1 = the most recently pushed sample)
    inline float read (float d) const noexcept
    {
        const float pos = (float) w - d;
        const int i = (int) std::floor (pos);
        const float f = pos - (float) i;
        const float y0 = buf[(size_t) ((i - 1) & mask)], y1 = buf[(size_t) (i & mask)];
        const float y2 = buf[(size_t) ((i + 1) & mask)], y3 = buf[(size_t) ((i + 2) & mask)];
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
};

// ===========================================================================
// YIN pitch tracker running on a decimated copy of the signal.
class PitchTracker
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        dec = juce::jmax (1, (int) std::round (sr / 22050.0));
        dsr = sr / dec;
        W = (int) (dsr * 0.028);
        maxLag = (int) (dsr / 70.0);
        minLag = juce::jmax (2, (int) (dsr / 1000.0));
        hop = juce::jmax (16, (int) (dsr * 0.004));
        ring.assign ((size_t) (W + maxLag), 0.0f);
        lin.assign ((size_t) (W + maxLag), 0.0f);
        diff.assign ((size_t) maxLag + 2, 0.0f);
        aa1.set (Biquad::LowPass, sr, 900.0f, 0.707f);  // vocal fundamentals live below ~1k
        aa2.set (Biquad::LowPass, sr, 900.0f, 0.707f);
        pos = 0; decCount = 0; hopCount = 0;
    }

    // Returns true when a new analysis frame was produced.
    bool push (float x)
    {
        const float y = aa2.process (aa1.process (x));
        if (++decCount < dec) return false;
        decCount = 0;
        ring[(size_t) pos] = y;
        pos = (pos + 1) % (int) ring.size();
        if (++hopCount < hop) return false;
        hopCount = 0;
        analyse();
        return true;
    }

    float freq = 0.0f;      // Hz, 0 when unvoiced
    float confidence = 0.0f;
    // Distance (full-rate samples) from the newest sample back to the centre of the analysis window
    int centreOffset() const { return (int) ((W + maxLag) * 0.5 * dec); }

private:
    void analyse()
    {
        const int N = (int) ring.size();
        double energy = 0;
        for (int i = 0; i < N; ++i)
        {
            lin[(size_t) i] = ring[(size_t) ((pos + i) % N)];
            energy += lin[(size_t) i] * lin[(size_t) i];
        }
        const float rms = (float) std::sqrt (energy / N);
        if (rms < 0.003f) { freq = 0; confidence = 0; return; }

        const float* x = lin.data();
        diff[0] = 1.0f;
        double running = 0;
        int best = -1;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            float s = 0;
            for (int j = 0; j < W; ++j)
            {
                const float d = x[j] - x[j + tau];
                s += d * d;
            }
            running += s;
            diff[(size_t) tau] = running > 0 ? (float) (s * tau / running) : 1.0f;
        }
        const float thresh = 0.15f;
        for (int tau = minLag; tau < maxLag; ++tau)
        {
            if (diff[(size_t) tau] < thresh)
            {
                while (tau + 1 < maxLag && diff[(size_t) tau + 1] < diff[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0)
        {
            // fall back to the global minimum if it's reasonably periodic
            float mn = 1e9f;
            for (int tau = minLag; tau < maxLag; ++tau)
                if (diff[(size_t) tau] < mn) { mn = diff[(size_t) tau]; best = tau; }
            if (mn > 0.35f) { freq = 0; confidence = 0; return; }
        }
        float t = (float) best;
        if (best > 1 && best < maxLag)
        {
            const float a = diff[(size_t) best - 1], b = diff[(size_t) best], c = diff[(size_t) best + 1];
            const float den = a - 2 * b + c;
            if (std::abs (den) > 1e-9f) t += 0.5f * (a - c) / den;
        }
        confidence = 1.0f - juce::jlimit (0.0f, 1.0f, diff[(size_t) best]);
        freq = (float) (dsr / t);
    }

    double sr = 44100, dsr = 22050;
    int dec = 2, W = 600, maxLag = 300, minLag = 20, hop = 88;
    std::vector<float> ring, lin, diff;
    Biquad aa1, aa2;
    int pos = 0, decCount = 0, hopCount = 0;
};

// ===========================================================================
// Pitch correction: YIN detection + scale quantisation + TD-PSOLA resynthesis
// (formant-preserving, with optional formant shift by resampling grains).
class Tuner
{
public:
    void prepare (double sampleRate, bool lowLatency)
    {
        sr = sampleRate;
        tracker.prepare (sr);
        const int maxL = (int) std::ceil (sr * 0.045) + 8;
        int sz = 1; while (sz < maxL * 4) sz <<= 1;
        inBuf.assign ((size_t) sz, 0.0f); acc.assign ((size_t) sz, 0.0f); norm.assign ((size_t) sz, 0.0f);
        mask = sz - 1;
        setLowLatency (lowLatency);
        reset();
    }

    void setLowLatency (bool ll)
    {
        minFreq = ll ? 110.0f : 75.0f;
        // grain support needs (1.5 + formantFactor) periods of lookahead; formant factor <= 1.42
        latency = (int) std::ceil (sr * 2.95 / minFreq) + 4;
    }
    int getLatency() const { return latency; }

    void reset()
    {
        std::fill (inBuf.begin(), inBuf.end(), 0.0f);
        std::fill (acc.begin(), acc.end(), 0.0f);
        std::fill (norm.begin(), norm.end(), 0.0f);
        n = 0; synthPos = 0; anaMark = 0; frameCount = 0; frameHead = 0;
        smoothedShift = 0; avgMidi = -1; currentNote = -1;
        frames.fill ({});
    }

    struct Settings { float amount = 1, speedMs = 10, humanize = 0, formantSt = 0, transposeSt = 0; int key = 0, scale = 0; bool enabled = true; };

    // UI read-outs
    std::atomic<float> uiDetectedMidi { -1.0f }, uiTargetMidi { -1.0f }, uiOutMidi { -1.0f };

    float process (float x, const Settings& s)
    {
        inBuf[(size_t) (n & mask)] = x;

        if (tracker.push (x))
            newFrame (s);

        const long long tOut = n - latency;

        // schedule grains so every output sample is covered
        while (synthPos <= (double) tOut + curPeriod (synthPos))
            placeGrain (s);

        const size_t o = (size_t) (tOut & mask);
        const float y = tOut >= 0 ? acc[o] / std::max (norm[o], 0.75f) : 0.0f;
        acc[o] = 0; norm[o] = 0;
        ++n;
        return y;
    }

private:
    struct Frame { long long centre = -1; float period = 0; float ratio = 1; bool voiced = false; };

    void newFrame (const Settings& s)
    {
        Frame f;
        f.centre = n - tracker.centreOffset();
        const float hz = tracker.freq;
        const float hopSec = 0.004f;
        f.voiced = hz >= minFreq && hz < 1000.0f && tracker.confidence > 0.6f;

        float shiftSt = 0;
        if (f.voiced)
        {
            const float midi = 69.0f + 12.0f * std::log2 (hz / 440.0f);
            avgMidi = avgMidi < 0 ? midi : avgMidi + (midi - avgMidi) * (1.0f - std::exp (-hopSec / 0.12f));

            if (s.enabled)
            {
                const int target = quantise (midi, s.key, s.scale);
                // humanize keeps natural vibrato by correcting the slow average instead of every wiggle
                const float errFast = (float) target - midi;
                const float errSlow = (float) target - avgMidi;
                const float err = errFast * (1.0f - s.humanize) + errSlow * s.humanize;
                shiftSt = err * s.amount;
                uiTargetMidi.store ((float) target);
            }
            uiDetectedMidi.store (midi);
            f.period = (float) (sr / hz);
        }
        else
        {
            avgMidi = -1; currentNote = -1;
            uiDetectedMidi.store (-1.0f); uiTargetMidi.store (-1.0f);
            f.period = (float) (sr * 0.008);
        }

        // retune speed smoothing (time constant in ms). 0 ms = instant snap.
        const float a = s.speedMs <= 0.01f ? 1.0f : 1.0f - std::exp (-hopSec / (s.speedMs * 0.001f));
        const float targetShift = f.voiced ? shiftSt : smoothedShift * 0.5f; // relax during consonants
        smoothedShift += (targetShift - smoothedShift) * (f.voiced ? a : 0.3f);

        const float total = f.voiced ? smoothedShift + s.transposeSt : 0.0f;
        f.ratio = std::pow (2.0f, total / 12.0f);
        uiOutMidi.store (f.voiced ? uiDetectedMidi.load() + total : -1.0f);

        frames[(size_t) frameHead] = f;
        frameHead = (frameHead + 1) % (int) frames.size();
        frameCount = std::min (frameCount + 1, (int) frames.size());
    }

    int quantise (float midi, int key, int scale)
    {
        const int mask12 = scaleMask (scale);
        auto allowed = [&] (int note) { return (mask12 >> (((note - key) % 12 + 12) % 12)) & 1; };
        // hysteresis: stay on the current note unless we've clearly moved away
        if (currentNote >= 0 && allowed (currentNote) && std::abs (midi - (float) currentNote) < 0.53f)
            return currentNote;
        const int base = (int) std::round (midi);
        int best = base; float bestD = 1e9f;
        for (int d = -6; d <= 6; ++d)
        {
            const int c = base + d;
            if (! allowed (c)) continue;
            const float dist = std::abs ((float) c - midi);
            if (dist < bestD) { bestD = dist; best = c; }
        }
        currentNote = best;
        return best;
    }

    const Frame& frameAt (double t) const
    {
        // newest frame whose centre is <= t (frames are time ordered)
        static const Frame unvoiced { -1, 0, 1, false };
        if (frameCount == 0) return unvoiced;
        const int N = (int) frames.size();
        for (int i = 1; i <= frameCount; ++i)
        {
            const Frame& f = frames[(size_t) ((frameHead - i + N) % N)];
            if ((double) f.centre <= t) return f;
        }
        return frames[(size_t) ((frameHead - frameCount + N) % N)];
    }

    double curPeriod (double t) const
    {
        const Frame& f = frameAt (t);
        return f.period > 0 ? f.period : sr * 0.008;
    }

    void placeGrain (const Settings& s)
    {
        const Frame& f = frameAt (synthPos);
        const double P = juce::jlimit (sr / 1000.0, sr / (double) minFreq, f.period > 0 ? (double) f.period : sr * 0.008);
        const double ratio = f.voiced ? juce::jlimit (0.5, 2.0, (double) f.ratio) : 1.0;
        const double formant = f.voiced ? std::pow (2.0, juce::jlimit (-6.0f, 6.0f, s.formantSt) / 12.0) : 1.0;

        // nearest analysis mark (marks advance one period at a time)
        if (std::abs (anaMark - synthPos) > P * 4.0 || (std::abs (ratio - 1.0) < 1e-9 && std::abs (formant - 1.0) < 1e-9))
            anaMark = synthPos; // re-sync (after silence, or when not shifting => transparent)
        while (anaMark + P * 0.5 < synthPos) anaMark += P;
        while (anaMark - P * 0.5 > synthPos) anaMark -= P;

        const long long newest = n;
        const double centre = synthPos;
        const long long first = std::max ((long long) std::ceil (centre - P), n - latency); // earlier ones already emitted
        const long long last = (long long) std::floor (centre + P);
        for (long long oi = first; oi <= last; ++oi)
        {
            const double k = (double) oi - centre;
            const double wpos = (k + P) / (2.0 * P);
            if (wpos <= 0.0 || wpos >= 1.0) continue;
            const float w = 0.5f - 0.5f * std::cos (2.0f * kPi * (float) wpos);
            const double srcT = anaMark + k * formant;
            if (srcT >= (double) newest - 1 || srcT < (double) (newest - (long long) inBuf.size() + 4)) continue;
            const long long si = (long long) std::floor (srcT);
            const float fr = (float) (srcT - (double) si);
            const float a = inBuf[(size_t) (si & mask)], b = inBuf[(size_t) ((si + 1) & mask)];
            acc[(size_t) (oi & mask)] += (a + (b - a) * fr) * w;
            norm[(size_t) (oi & mask)] += w;
        }
        synthPos += P / ratio;
    }

    double sr = 44100;
    PitchTracker tracker;
    std::vector<float> inBuf, acc, norm;
    long long mask = 0, n = 0;
    double synthPos = 0, anaMark = 0;
    int latency = 2048;
    float minFreq = 75.0f;
    std::array<Frame, 64> frames {};
    int frameHead = 0, frameCount = 0;
    float smoothedShift = 0, avgMidi = -1;
    int currentNote = -1;
};

// ===========================================================================
class Gate
{
public:
    void prepare (double s) { sr = s; env = 0; gain = 1; hold = 0; }
    float process (float x, float threshDb, float rangeDb, float attMs, float relMs)
    {
        const float a = std::abs (x);
        env = a > env ? a : env * envRel;
        const bool open = gainToDb (env) > threshDb;
        if (open) hold = (int) (sr * 0.03);
        const float target = (open || hold-- > 0) ? 1.0f : dbToGain (-rangeDb);
        const float c = target > gain ? msCoeff (attMs, sr) : msCoeff (relMs, sr);
        gain = target + (gain - target) * c;
        return x * gain;
    }
    void setup() { envRel = msCoeff (40.0f, sr); }
    float env = 0, gain = 1;
private:
    double sr = 44100; float envRel = 0.999f; int hold = 0;
};

// ===========================================================================
class Compressor
{
public:
    void prepare (double s) { sr = s; env = 0; grDb = 0; }
    float process (float x, float thr, float ratio, float attMs, float relMs, float makeupDb, float mix, float punch)
    {
        const float lvl = gainToDb (std::abs (x));
        env = lvl > env ? lvl + (env - lvl) * msCoeff (attMs, sr) : lvl + (env - lvl) * msCoeff (relMs, sr);
        const float knee = 6.0f;
        const float over = env - thr;
        float gr = 0;
        if (over > knee * 0.5f) gr = over * (1.0f - 1.0f / ratio);
        else if (over > -knee * 0.5f) { const float t = over + knee * 0.5f; gr = (1.0f - 1.0f / ratio) * t * t / (2.0f * knee); }
        grDb = gr * punch;
        const float wet = x * dbToGain (-grDb + makeupDb);
        return x + (wet - x) * mix;
    }
    float env = -120, grDb = 0;
private:
    double sr = 44100;
};

// ===========================================================================
class DeEsser
{
public:
    void prepare (double s)
    {
        sr = s;
        juce::dsp::ProcessSpec spec { s, 1, 1 };
        lp.prepare (spec); hp.prepare (spec);
        lp.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        hp.setType (juce::dsp::LinkwitzRileyFilterType::highpass);
        env = 0; grDb = 0;
    }
    void setFreq (float f) { if (std::abs (f - freq) > 0.5f) { freq = f; lp.setCutoffFrequency (f); hp.setCutoffFrequency (f); } }
    float process (float x, float thr, float range)
    {
        const float lo = lp.processSample (0, x);
        const float hi = hp.processSample (0, x);
        const float a = std::abs (hi);
        env = a > env ? a + (env - a) * att : a + (env - a) * rel;
        const float over = gainToDb (env) - thr;
        const float target = juce::jlimit (0.0f, range, over * 0.75f);
        grDb += (target - grDb) * 0.02f;
        return lo + hi * dbToGain (-grDb);
    }
    void setup() { att = msCoeff (0.5f, sr); rel = msCoeff (60.0f, sr); }
    float grDb = 0;
private:
    double sr = 44100; float env = 0, att = 0.9f, rel = 0.999f, freq = -1;
    juce::dsp::LinkwitzRileyFilter<float> lp, hp;
};

// ===========================================================================
class Drive
{
public:
    void prepare (double s) { sr = s; lp = 0; lpC = std::exp (-2.0f * kPi * 2500.0f / (float) s); dc = 0; dcPrev = 0; }
    static float shape (float x, float g)
    {
        // asymmetric soft clip: a touch of even harmonics for warmth
        return (std::tanh (g * x + 0.12f) - std::tanh (0.12f)) * std::pow (g, -0.8f);
    }
    float process (float x, float amount, float tone, float mix)
    {
        const float g = 1.0f + amount * 15.0f;
        float y = shape (x, g);
        // DC blocker
        const float out = y - dcPrev + 0.995f * dc; dcPrev = y; dc = out; y = out;
        // tilt tone
        lp = y + (lp - y) * lpC;
        const float hpart = y - lp;
        y = lp * (1.0f - 0.5f * tone) + hpart * (1.0f + tone);
        return x + (y - x) * mix;
    }
private:
    double sr = 44100; float lp = 0, lpC = 0.7f, dc = 0, dcPrev = 0;
};

// ===========================================================================
// Mono -> stereo double tracker: two modulated delay voices panned L/R.
class Doubler
{
public:
    void prepare (double s) { sr = s; dl.prepare ((int) (s * 0.1)); ph1 = 0; ph2 = 0.37f; ph3 = 0.71f; }
    void process (float in, float& L, float& R, float mix, float detuneCt, float delayMs)
    {
        dl.push (in);
        const float dev = detuneCt * 0.000578f;                 // pitch deviation ratio
        const float r1 = 0.31f, r2 = 0.47f, r3 = 0.19f;
        const float depth = dev / (2.0f * kPi * 0.35f) * (float) sr; // samples
        ph1 += r1 / (float) sr; if (ph1 > 1) ph1 -= 1;
        ph2 += r2 / (float) sr; if (ph2 > 1) ph2 -= 1;
        ph3 += r3 / (float) sr; if (ph3 > 1) ph3 -= 1;
        const float base = delayMs * 0.001f * (float) sr;
        const float mL = 0.6f * std::sin (2 * kPi * ph1) + 0.4f * std::sin (2 * kPi * ph3);
        const float mR = 0.6f * std::sin (2 * kPi * ph2) - 0.4f * std::sin (2 * kPi * (ph3 + 0.25f));
        const float vL = dl.read (std::max (2.0f, base + depth * (1.0f + mL)));
        const float vR = dl.read (std::max (2.0f, base * 1.41f + depth * (1.0f + mR)));
        const float g = mix * 0.85f;
        L += vL * g; R += vR * g;
    }
private:
    double sr = 44100; DelayLine dl; float ph1 = 0, ph2 = 0, ph3 = 0;
};

// ===========================================================================
class Chorus
{
public:
    void prepare (double s) { sr = s; l.prepare ((int) (s * 0.06)); r.prepare ((int) (s * 0.06)); ph = 0; }
    void process (float& L, float& R, float rate, float depth, float mix)
    {
        l.push (L); r.push (R);
        ph += rate / (float) sr; if (ph > 1) ph -= 1;
        const float base = 0.012f * (float) sr, dep = depth * 0.006f * (float) sr;
        const float wl = l.read (base + dep * (0.5f + 0.5f * std::sin (2 * kPi * ph)));
        const float wr = r.read (base + dep * (0.5f + 0.5f * std::sin (2 * kPi * (ph + 0.25f))));
        L = L * (1.0f - 0.5f * mix) + wl * mix * 0.7f;
        R = R * (1.0f - 0.5f * mix) + wr * mix * 0.7f;
    }
private:
    double sr = 44100; DelayLine l, r; float ph = 0;
};

// ===========================================================================
class StereoDelay
{
public:
    void prepare (double s)
    {
        sr = s; l.prepare ((int) (s * 2.1)); r.prepare ((int) (s * 2.1));
        smoothedTime = -1;
    }
    void setFilters (float lowCut, float highCut)
    {
        for (auto* b : { &hpL, &hpR }) { auto z1 = b->z1, z2 = b->z2; b->set (Biquad::HighPass, sr, lowCut, 0.6f); b->z1 = z1; b->z2 = z2; }
        for (auto* b : { &lpL, &lpR }) { auto z1 = b->z1, z2 = b->z2; b->set (Biquad::LowPass, sr, highCut, 0.6f); b->z1 = z1; b->z2 = z2; }
    }
    void process (float& L, float& R, float timeMs, float fb, float mix, bool ping)
    {
        const float target = juce::jlimit (1.0f, (float) (sr * 2.0), timeMs * 0.001f * (float) sr);
        if (smoothedTime < 0) smoothedTime = target;
        smoothedTime += (target - smoothedTime) * 0.0005f;
        const float dL = l.read (smoothedTime), dR = r.read (smoothedTime);
        float fL = lpL.process (hpL.process (dL)), fR = lpR.process (hpR.process (dR));
        if (ping)
        {
            const float mono = 0.5f * (L + R);
            l.push (mono + fR * fb);
            r.push (fL * fb);
        }
        else
        {
            l.push (L + fL * fb);
            r.push (R + fR * fb);
        }
        tapL = dL; tapR = dR;
        L += dL * mix; R += dR * mix;
    }
    float tapL = 0, tapR = 0;
private:
    double sr = 44100; DelayLine l, r; Biquad hpL, hpR, lpL, lpR; float smoothedTime = -1;
};

// ===========================================================================
class Verb
{
public:
    void prepare (double s, int maxBlock)
    {
        sr = s;
        verb.setSampleRate (s);
        pre[0].prepare ((int) (s * 0.25)); pre[1].prepare ((int) (s * 0.25));
        wet.setSize (2, maxBlock);
    }
    void setParams (float size, float damp, float width, float lowCut)
    {
        juce::Reverb::Parameters p;
        p.roomSize = 0.3f + 0.69f * size;
        p.damping = damp;
        p.width = width;
        p.wetLevel = 1.0f; p.dryLevel = 0.0f; p.freezeMode = 0.0f;
        verb.setParameters (p);
        for (auto& b : hp) { auto z1 = b.z1, z2 = b.z2; b.set (Biquad::HighPass, sr, lowCut, 0.7f); b.z1 = z1; b.z2 = z2; }
    }
    void process (float* L, float* R, int num, float preDelayMs, float mix)
    {
        auto* wl = wet.getWritePointer (0); auto* wr = wet.getWritePointer (1);
        const float pd = std::max (1.0f, preDelayMs * 0.001f * (float) sr);
        for (int i = 0; i < num; ++i)
        {
            pre[0].push (L[i]); pre[1].push (R[i]);
            wl[i] = hp[0].process (pre[0].read (pd));
            wr[i] = hp[1].process (pre[1].read (pd));
        }
        verb.processStereo (wl, wr, num);
        for (int i = 0; i < num; ++i)
        {
            L[i] += wl[i] * mix * 0.6f;
            R[i] += wr[i] * mix * 0.6f;
        }
    }
    void reset() { verb.reset(); pre[0].reset(); pre[1].reset(); for (auto& b : hp) b.reset(); }
private:
    double sr = 44100; juce::Reverb verb; DelayLine pre[2]; Biquad hp[2]; juce::AudioBuffer<float> wet;
};

// ===========================================================================
// Peak limiter with optional lookahead.
class Limiter
{
public:
    void prepare (double s) { sr = s; look = (int) (s * 0.002); dl[0].prepare (look + 4); dl[1].prepare (look + 4); hist.assign ((size_t) look + 1, 1.0f); hp = 0; g = 1; }
    int lookahead (bool lowLatency) const { return lowLatency ? 0 : look; }
    void process (float& L, float& R, float gainDb, float ceilDb, float relMs, bool lowLatency)
    {
        const float pre = dbToGain (gainDb), ceil = dbToGain (ceilDb);
        L *= pre; R *= pre;
        const float pk = std::max (std::abs (L), std::abs (R));
        const float need = pk > ceil ? ceil / pk : 1.0f;
        float target = need;
        if (! lowLatency)
        {
            hist[(size_t) hp] = need; hp = (hp + 1) % (int) hist.size();
            for (auto v : hist) target = std::min (target, v);
            dl[0].push (L); dl[1].push (R);
            L = dl[0].read ((float) look + 1.0f); R = dl[1].read ((float) look + 1.0f);
        }
        const float att = lowLatency ? 0.0f : std::exp (-1.0f / ((float) look / 3.0f));
        g = target < g ? target + (g - target) * att : target + (g - target) * msCoeff (relMs, sr);
        L = juce::jlimit (-ceil, ceil, L * g);
        R = juce::jlimit (-ceil, ceil, R * g);
        grDb = -gainToDb (g);
    }
    float grDb = 0;
private:
    double sr = 44100; int look = 96; DelayLine dl[2]; std::vector<float> hist; int hp = 0; float g = 1;
};

// ===========================================================================
// Lock-free-ish scope buffer for the UI (tearing is harmless for display).
struct ScopeBuffer
{
    static constexpr int size = 8192;
    std::array<float, size> l {}, r {};
    std::atomic<int> write { 0 };
    void push (float a, float b) { const int w = write.load (std::memory_order_relaxed); l[(size_t) w] = a; r[(size_t) w] = b; write.store ((w + 1) & (size - 1), std::memory_order_release); }
    void copyLast (int num, float* outL, float* outR) const
    {
        const int w = write.load (std::memory_order_acquire);
        for (int i = 0; i < num; ++i)
        {
            const int idx = (w - num + i + size * 2) & (size - 1);
            if (outL) outL[i] = l[(size_t) idx];
            if (outR) outR[i] = r[(size_t) idx];
        }
    }
};
} // namespace glaze
