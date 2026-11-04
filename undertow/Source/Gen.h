#pragma once
// Undertow bassline generator. Pure, deterministic: same params + seeds = same pattern.
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace ut
{
struct Note
{
    int step = 0;      // 16th index from bar 1
    int len = 1;       // in 16ths
    int deg = 0;       // scale degree relative to root (can be negative / > 7)
    int vel = 100;
    bool slide = false; // glide into this note
};

enum Style { Rolling, Bouncy, Offbeat, TwoStep, Minimal, Driving, Broken, Acid, NumStyles };
inline const char* styleName (int s)
{
    static const char* n[] = { "ROLLING", "BOUNCY", "OFFBEAT", "2-STEP", "MINIMAL", "DRIVING", "BROKEN", "ACID" };
    return n[s];
}
inline const char* styleBlurb (int s)
{
    static const char* n[] = { "16ths rolling between the kicks", "short notes, octave jumps", "classic house offbeat",
                               "UKG swing, long notes + slides", "sparse and hypnotic", "every 16th that isn't a kick",
                               "syncopated, off the grid", "squelchy 16ths with slides" };
    return n[s];
}

enum Scale { Minor, Dorian, Phrygian, HarmMinor, Major, NumScales };
inline const char* scaleName (int s) { static const char* n[] = { "Minor", "Dorian", "Phrygian", "Harm. Minor", "Major" }; return n[s]; }
inline const int* scaleSteps (int s)
{
    static const int sc[][7] = { { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 4, 5, 7, 9, 11 } };
    return sc[s];
}
inline int degToSemi (int deg, int scale)
{
    const int o = (int) std::floor (deg / 7.0); const int d = deg - o * 7;
    return scaleSteps (scale)[d] + 12 * o;
}

struct Params
{
    int style = Rolling, bars = 2;
    float density = 0.6f, movement = 0.45f, variation = 0.3f, gate = 0.8f;
    uint32_t rhythmSeed = 1, pitchSeed = 1, mutateSeed = 0;
    int mutations = 0;
};

// onset probability per 16th in one bar (kick on 0, 4, 8, 12)
inline void rhythmTemplate (int style, float* p)
{
    static const float T[][16] = {
        { .02f, .55f, .95f, .6f, .02f, .55f, .95f, .6f, .02f, .55f, .95f, .6f, .02f, .55f, .95f, .6f },  // rolling
        { 0, .1f, .95f, .35f, 0, .45f, .7f, .2f, 0, .1f, .95f, .45f, 0, .5f, .6f, .3f },                // bouncy
        { 0, 0, 1, .1f, 0, 0, 1, .15f, 0, 0, 1, .1f, 0, 0, 1, .3f },                                     // offbeat
        { .9f, 0, 0, .45f, 0, 0, .6f, 0, 0, .1f, .9f, 0, 0, .55f, 0, .3f },                              // 2-step
        { 0, 0, .75f, 0, 0, 0, .1f, .35f, 0, 0, .55f, 0, 0, .3f, 0, .15f },                              // minimal
        { .05f, .85f, .95f, .85f, .05f, .85f, .95f, .85f, .05f, .85f, .95f, .85f, .05f, .85f, .95f, .85f }, // driving
        { .1f, .4f, .3f, .6f, .1f, .25f, .55f, .4f, .15f, .45f, .3f, .5f, .1f, .55f, .35f, .45f },       // broken
        { .7f, .6f, .8f, .6f, .7f, .6f, .8f, .6f, .7f, .6f, .8f, .6f, .7f, .6f, .8f, .6f } };           // acid
    for (int i = 0; i < 16; ++i) p[i] = T[style][i];
}
inline int maxLen (int style) { static const int m[] = { 2, 1, 2, 4, 2, 1, 3, 2 }; return m[style]; }

inline std::vector<Note> generate (const Params& P)
{
    std::mt19937 rr (P.rhythmSeed * 2654435761u + (uint32_t) P.style * 97u), pr (P.pitchSeed * 40503u + 7u);
    auto u = [] (std::mt19937& g) { return std::uniform_real_distribution<float> (0.f, 1.f) (g); };
    float tp[16]; rhythmTemplate (P.style, tp);
    const int total = P.bars * 16;

    // --- rhythm: build bar 1 (and bar 3), later bars repeat with variation
    std::vector<int> onsets;
    const float dens = (0.15f + P.density * 1.25f) * (P.style == Rolling ? 1.5f : 1.f);
    auto barOnsets = [&] () {
        std::vector<int> o;
        for (int s = 0; s < 16; ++s) if (u (rr) < std::min (1.f, std::pow (tp[s], 1.f / dens) * (0.35f + 0.75f * P.density))) o.push_back (s);
        if (o.size() < 2) { o = { 2, 10 }; }
        return o;
    };
    std::vector<int> A = barOnsets(), B = A;
    // B: the answer bar, differs in the last beat (fills live here)
    for (int s = 12; s < 16; ++s)
    {
        if (u (rr) < 0.35f + P.variation * 0.5f)
        {
            auto it = std::find (B.begin(), B.end(), s);
            if (it != B.end()) B.erase (it);
            else if (tp[s] > 0.05f || P.style == Broken) B.push_back (s);
        }
    }
    std::sort (B.begin(), B.end());
    for (int bar = 0; bar < P.bars; ++bar)
    {
        auto src = (bar % 2 == 0) ? A : B;
        if (bar >= 2) // bars 3-4: small variation anywhere
            for (int s = 0; s < 12; ++s)
                if (u (rr) < P.variation * 0.18f)
                {
                    auto it = std::find (src.begin(), src.end(), s);
                    if (it != src.end()) { if (src.size() > 2) src.erase (it); }
                    else if (tp[s] > 0.2f) src.push_back (s);
                }
        std::sort (src.begin(), src.end());
        for (int s : src) onsets.push_back (bar * 16 + s);
    }

    // --- pitch
    // weighted palette of scale degrees; movement pushes weight off the root
    struct W { int deg; float w; };
    const float m = P.movement;
    std::vector<W> pal = { { 0, 3.2f - 2.0f * m }, { 7, 0.5f + 0.9f * m }, { 4, 0.4f + 0.9f * m }, { 6, 0.2f + 0.8f * m },
                           { 2, 0.15f + 0.7f * m }, { -1, 0.25f + 0.6f * m }, { 3, 0.02f + 0.4f * m }, { -3, std::max (0.f, m - 0.4f) * 0.6f } };
    if (P.style == Bouncy) pal[1].w += 1.6f;           // octave jumps
    if (P.style == Minimal) { pal[0].w += 1.5f; }
    if (P.style == Acid) { pal[1].w += 0.8f; pal[3].w += 0.5f; }
    float tot = 0; for (auto& p : pal) tot += p.w;
    auto pick = [&] () { float x = u (pr) * tot; for (auto& p : pal) { if ((x -= p.w) <= 0) return p.deg; } return 0; };
    // a motif per 2 bars so it sounds written, not random
    std::vector<int> motif; for (int i = 0; i < 64; ++i) motif.push_back (pick());

    std::vector<Note> out;
    for (size_t i = 0; i < onsets.size(); ++i)
    {
        Note n; n.step = onsets[i];
        const int next = i + 1 < onsets.size() ? onsets[i + 1] : total;
        const int gap = next - n.step;
        int L = std::min (gap, maxLen (P.style));
        if (P.style == TwoStep && u (pr) < 0.4f) L = std::min (gap, 4);
        n.len = std::max (1, (int) std::round (L * (0.4f + 0.6f * P.gate)));
        const int inBar = n.step % 16, bar = n.step / 16;
        // first hit after the kick lands on the root more often
        int d = motif[(size_t) (inBar + (bar % 2) * 16) % motif.size()];
        if ((inBar == 2 || (inBar < 4 && ! (i > 0 && out.back().step / 16 == bar))) && u (pr) < 0.85f - 0.35f * m) d = 0; // anchor the bar on the root
        if (bar == P.bars - 1 && inBar >= 12 && u (pr) < 0.5f) d = (u (pr) < 0.5f ? 4 : -1); // turnaround
        n.deg = d;
        const bool off = (inBar % 4) == 2;
        n.vel = std::clamp ((int) ((off ? 112 : 92) + (u (pr) - 0.5f) * 20), 60, 127);
        n.slide = (P.style == Acid || P.style == TwoStep) ? u (pr) < 0.25f + 0.2f * m : u (pr) < 0.06f * m;
        if (i > 0 && out.back().step + out.back().len < n.step) n.slide = false; // only slide when legato
        out.push_back (n);
    }
    // legato for slides
    for (size_t i = 0; i + 1 < out.size(); ++i)
        if (out[i + 1].slide) out[i].len = out[i + 1].step - out[i].step;

    // --- mutations (MUTATE button): nudge a few notes
    if (P.mutations > 0 && ! out.empty())
    {
        std::mt19937 mr (P.mutateSeed);
        for (int k = 0; k < P.mutations * 3; ++k)
        {
            auto& n = out[mr() % out.size()];
            switch (mr() % 3)
            {
                case 0: n.deg = pick(); break;
                case 1: n.deg += (n.deg >= 7 ? -7 : 7); break;
                default: n.vel = std::clamp (n.vel + (int) (mr() % 41) - 20, 60, 127);
            }
        }
    }
    return out;
}
} // namespace ut
