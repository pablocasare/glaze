#pragma once
// Undertow bassline generator. Pure, deterministic: same params + seeds = same pattern.
//
// How a line is written:
//   1. Rhythm: a one-beat cell (or a whole-bar groove for 2-step style genres) repeats as the hook.
//   2. Phrasing: bar 2 answers bar 1 with a fill on beat 4; over 4 bars, bar 3 varies the hook and
//      bar 4 turns the phrase around with a bigger fill.
//   3. Harmony: each bar follows a chord (CHORDS); the last note of a bar walks into the next root.
//   4. Detail: ghost notes as pickups, accents on the offbeats, slides where the style wants them.
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>

namespace ut
{
struct Note
{
    int step = 0;       // 16th index from bar 1
    int len = 1;        // in 16ths
    int deg = 0;        // scale degree relative to the key root (can be negative / > 7)
    int vel = 100;
    bool slide = false; // glide into this note
    bool ghost = false;
};

enum Style { Rolling, Bouncy, Offbeat, TwoStep, Minimal, Driving, Broken, Acid, SpeedGarage, Afro, DeepTech, Bassline, NumStyles };
inline const char* styleName (int s)
{
    static const char* n[] = { "ROLLING", "BOUNCY", "OFFBEAT", "2-STEP", "MINIMAL", "DRIVING", "BROKEN", "ACID", "SPEED GARAGE", "AFRO", "DEEP TECH", "BASSLINE" };
    return n[s];
}
inline const char* styleBlurb (int s)
{
    static const char* n[] = { "16ths rolling between the kicks", "short notes, octave jumps", "classic house offbeat",
                               "UKG swing, long notes + slides", "sparse and hypnotic", "every 16th that isn't a kick",
                               "syncopated, off the grid", "squelchy 16ths with slides", "octave wobble, made for a Reese",
                               "tresillo 3-3-2 bounce", "long subs that slide", "UK bassline house, bouncy octaves" };
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

// chord progressions as the bass root's scale degree per bar (bass-friendly inversions: VI and VII below the root)
enum Chords { ChordAuto, ChordStay, ChordI_VI, ChordI_VII, ChordI_iv, ChordI_VI_VII, ChordI_iv_VI_V, ChordI_III_VII_VI, NumChords };
inline const char* chordName (int c)
{
    static const char* n[] = { "Auto", "Stay on i", "i - VI", "i - VII", "i - iv", "i - VI - VII", "i - iv - VI - V", "i - III - VII - VI" };
    return n[c];
}
inline const int* chordRoots (int c)
{
    static const int r[][4] = { { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, -2, 0, -2 }, { 0, -1, 0, -1 }, { 0, 3, 0, 3 }, { 0, -2, -1, 0 }, { 0, 3, -2, -3 }, { 0, 2, -1, -2 } };
    return r[c];
}

struct Params
{
    int style = Rolling, bars = 2, chords = ChordAuto;
    float density = 0.6f, movement = 0.45f, variation = 0.3f, gate = 0.8f, ghosts = 0.3f, accent = 0.5f;
    uint32_t rhythmSeed = 1, pitchSeed = 1, mutateSeed = 0, take = 0; // take: EVOLVE's new take on the variations each loop
    int mutations = 0;
};

// ---------------------------------------------------------------- building blocks
// A cell is a few notes; role picks the pitch: R root, O octave, 5 fifth, 7 b7 below, 3 third, 4 fourth, 6 b6 below, 2 second
struct Cn { int s, len; char r; };
using Cell = std::vector<Cn>;
inline int roleDeg (char r)
{
    switch (r) { case 'O': return 7; case '5': return 4; case '7': return -1; case '3': return 2; case '4': return 3; case '6': return -2; case '2': return 1; default: return 0; }
}
inline bool usesBeatCells (int style) { return style == Rolling || style == Bouncy || style == Offbeat || style == Driving || style == Acid || style == SpeedGarage || style == Bassline; }

// one-beat cells (offsets 0..3 inside the beat, 0 = on the kick)
inline const std::vector<Cell>& beatCells (int style)
{
    static const std::vector<Cell> rolling { { { 2, 2, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } },
                                             { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> offbeat { { { 2, 2, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 2, 1, 'R' } }, { { 2, 3, 'R' } }, { { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> bouncy { { { 2, 1, 'O' } }, { { 2, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'O' }, { 2, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> driving { { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> acid { { { 0, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 0, 2, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 2, 'O' } },
                                          { { 0, 1, 'R' }, { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 0, 1, 'R' }, { 1, 1, 'O' }, { 2, 1, 'R' }, { 3, 1, '5' } } };
    static const std::vector<Cell> speed { { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 2, 2, 'O' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> bassline { { { 2, 1, 'O' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'O' }, { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 2, 2, 'O' } } };
    switch (style)
    {
        case Offbeat: return offbeat; case Bouncy: return bouncy; case Driving: return driving; case Acid: return acid;
        case SpeedGarage: return speed; case Bassline: return bassline; default: return rolling;
    }
}
// last-beat fills: where the line moves
inline const std::vector<Cell>& fillCells (int style)
{
    static const std::vector<Cell> f { { { 2, 1, 'R' }, { 3, 1, '7' } }, { { 1, 1, 'R' }, { 2, 1, '5' }, { 3, 1, 'O' } }, { { 2, 1, '7' }, { 3, 1, 'R' } },
                                       { { 2, 1, '3' }, { 3, 1, '4' } }, { { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } }, { { 2, 1, '5' }, { 3, 1, '7' } } };
    static const std::vector<Cell> big { { { 0, 1, 'O' }, { 1, 1, '7' }, { 2, 1, '5' }, { 3, 1, '4' } }, { { 1, 1, '3' }, { 2, 1, '4' }, { 3, 1, '5' } },
                                         { { 0, 1, 'R' }, { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } }, { { 1, 1, '5' }, { 2, 1, '7' }, { 3, 1, 'O' } } };
    static const std::vector<Cell> off { { { 2, 1, 'R' }, { 3, 1, '5' } }, { { 2, 2, '7' } }, { { 2, 2, '5' } }, { { 2, 1, 'O' }, { 3, 1, '7' } } };
    static const std::vector<Cell> acid { { { 0, 1, 'O' }, { 1, 1, '7' }, { 2, 1, '5' }, { 3, 1, '3' } }, { { 0, 1, 'R' }, { 1, 1, '3' }, { 2, 1, '4' }, { 3, 1, '5' } }, { { 0, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, '7' } } };
    if (style == Offbeat || style == Minimal || style == DeepTech) return off;
    if (style == Acid) return acid;
    if (style == -1) return big;
    return f;
}
// whole-bar grooves for the styles that don't think in beats
inline const std::vector<Cell>& barTemplates (int style)
{
    static const std::vector<Cell> twoStep {
        { { 0, 3, 'R' }, { 3, 2, 'R' }, { 6, 2, 'R' }, { 10, 2, 'O' }, { 13, 1, 'R' }, { 14, 2, '7' } },
        { { 0, 2, 'R' }, { 3, 1, 'R' }, { 6, 3, '5' }, { 10, 2, 'R' }, { 13, 3, '3' } },
        { { 2, 2, 'R' }, { 6, 1, 'R' }, { 7, 2, 'O' }, { 10, 2, 'R' }, { 14, 2, '7' } },
        { { 0, 4, 'R' }, { 6, 2, 'R' }, { 10, 1, 'R' }, { 11, 2, 'O' }, { 14, 2, '5' } } };
    static const std::vector<Cell> minimal {
        { { 2, 1, 'R' }, { 10, 1, 'R' } }, { { 2, 1, 'R' }, { 7, 1, 'R' }, { 10, 1, 'R' } }, { { 3, 1, 'R' }, { 10, 2, 'R' }, { 13, 1, 'R' } },
        { { 2, 1, 'R' }, { 6, 1, 'O' }, { 10, 1, 'R' }, { 15, 1, 'R' } } };
    static const std::vector<Cell> broken {
        { { 1, 1, 'R' }, { 3, 1, 'R' }, { 6, 1, 'O' }, { 9, 2, 'R' }, { 12, 1, '5' }, { 14, 1, 'R' } },
        { { 0, 1, 'R' }, { 3, 2, 'R' }, { 7, 1, 'R' }, { 8, 1, 'O' }, { 11, 2, 'R' }, { 14, 1, '7' } },
        { { 2, 1, 'R' }, { 5, 1, 'R' }, { 6, 1, 'O' }, { 9, 1, 'R' }, { 11, 1, 'R' }, { 13, 2, '5' } } };
    static const std::vector<Cell> afro {   // tresillo: 3 + 3 + 2 sixteenths, twice
        { { 0, 2, 'R' }, { 3, 2, 'R' }, { 6, 2, '5' }, { 10, 2, 'R' }, { 12, 1, '7' }, { 14, 2, 'R' } },
        { { 0, 3, 'R' }, { 3, 3, 'R' }, { 6, 2, 'R' }, { 11, 2, 'O' }, { 14, 2, '5' } },
        { { 2, 1, 'R' }, { 3, 2, 'R' }, { 6, 1, 'R' }, { 8, 2, '5' }, { 11, 3, 'R' }, { 14, 1, '7' } } };
    static const std::vector<Cell> deep {
        { { 2, 3, 'R' }, { 10, 4, 'R' } }, { { 2, 2, 'R' }, { 6, 1, 'R' }, { 10, 3, 'R' }, { 14, 1, '7' } }, { { 3, 2, 'R' }, { 7, 1, 'R' }, { 10, 4, 'R' } },
        { { 2, 4, 'R' }, { 7, 1, 'O' }, { 10, 3, 'R' }, { 14, 2, '5' } } };
    switch (style) { case TwoStep: return twoStep; case Minimal: return minimal; case Afro: return afro; case DeepTech: return deep; default: return broken; }
}
inline float slideAmount (int style)
{
    switch (style) { case Acid: return 0.55f; case TwoStep: return 0.35f; case SpeedGarage: return 0.4f; case DeepTech: return 0.45f; case Bassline: return 0.2f; default: return 0.0f; }
}

// the bass root (scale degree) of each bar: the chord progression the line follows
inline void barRoots (const Params& P, int* shift)
{
    for (int b = 0; b < 4; ++b) shift[b] = 0;
    if (P.bars == 1) return;
    if (P.chords == ChordAuto)
    {
        std::mt19937 hr (P.pitchSeed * 2246822519u + 13u);
        if (std::uniform_real_distribution<float> (0.f, 1.f) (hr) < (P.movement - 0.2f) * 1.4f)
        {
            static const int prog[] = { ChordI_VI, ChordI_VII, ChordI_iv, ChordI_VI_VII };
            const int c = prog[hr() % 4];
            for (int b = 0; b < 4; ++b) shift[b] = chordRoots (c)[b];
        }
    }
    else for (int b = 0; b < 4; ++b) shift[b] = chordRoots (P.chords)[b];
    if (P.bars == 2 && P.chords == ChordI_VI_VII) shift[1] = -2;
}

inline std::vector<Note> generate (const Params& P)
{
    std::mt19937 rr (P.rhythmSeed * 2654435761u + (uint32_t) P.style * 97u), pr (P.pitchSeed * 40503u + 7u), gr (P.rhythmSeed * 69069u + 1u);
    auto u = [] (std::mt19937& g) { return std::uniform_real_distribution<float> (0.f, 1.f) (g); };
    const float m = P.movement, v = P.variation, d = P.density;
    const int total = P.bars * 16;
    // pitch palette: a cell's role is kept, or (more often with MOVEMENT) swapped for another chord tone
    const bool octavey = P.style == Bouncy || P.style == Bassline || P.style == SpeedGarage || P.style == Acid;
    auto rollRole = [&] (char r, std::mt19937& g) -> char {
        if (u (g) > 0.25f + m * 0.6f) return r;
        const float w[] = { 1.0f, octavey ? 0.9f : 0.35f, 0.45f * m + 0.05f, 0.4f * m + 0.05f, 0.3f * m, 0.25f * m, 0.12f * m, 0.12f * m };
        static const char rs[] = { 'R', 'O', '5', '7', '3', '4', '6', '2' };
        float t = 0; for (float x : w) t += x;
        float x = u (g) * t; for (int i = 0; i < 8; ++i) { x -= w[i]; if (x <= 0) return rs[i]; }
        return 'R'; };
    auto rollLen = [&] (int len, std::mt19937& g) { return (u (g) < 0.25f) ? std::max (1, len + ((g() & 1) ? 1 : -1)) : len; };
    struct Raw { int step, len; char r; bool fill; };
    std::vector<Raw> raw;
    using Bar = std::vector<Raw>; // one bar, steps 0..15
    const auto& fills = fillCells (P.style); const auto& bigFills = fillCells (-1);
    Bar hook;
    std::function<Bar (std::mt19937&, int)> varyBeats; // re-write n beats of the hook
    if (usesBeatCells (P.style))
    {
        const auto& cells = beatCells (P.style);
        // the hook is a whole bar made of 2-3 different one-beat cells in a form (AAAB, ABAB, AABA, ABAC, ABBA)
        auto pick = [&] () { // denser cells more likely with DENSITY
            std::vector<float> w; for (auto& c : cells) w.push_back (0.25f + std::pow (std::max (0.05f, 1.f - std::abs ((float) c.size() / 3.f - d * 1.2f)), 2.f));
            float t = 0; for (float x : w) t += x; float x = u (rr) * t;
            for (size_t i = 0; i < w.size(); ++i) { x -= w[i]; if (x <= 0) return (int) i; } return 0; };
        int X = pick(), Y = pick(), Z = pick(); for (int k = 0; Y == X && k < 6; ++k) Y = pick(); for (int k = 0; (Z == X || Z == Y) && k < 6; ++k) Z = pick();
        static const int forms[][4] = { { 0, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 0 }, { 0, 1, 0, 2 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 } };
        const int form = (int) (rr() % (d < 0.35f ? 6 : 5));
        const int ids[] = { X, Y, Z };
        for (int beat = 0; beat < 4; ++beat)
            for (auto& n : cells[(size_t) ids[forms[form][beat]]]) hook.push_back ({ beat * 4 + n.s, n.len, n.r, false });
        for (auto& n : hook) n.r = rollRole (n.r, pr);
        // keep a strong first note: beat 1's first hit is the root
        if (! hook.empty()) hook[0].r = 'R';
        varyBeats = [&cells, &hook, &rollRole, &pr, u] (std::mt19937& g, int nBeats) {
            Bar b = hook;
            for (int k = 0; k < nBeats; ++k)
            {
                const int beat = 1 + (int) (g() % 3);
                b.erase (std::remove_if (b.begin(), b.end(), [&] (const Raw& r) { return r.step / 4 == beat; }), b.end());
                for (auto& n : cells[g() % cells.size()]) b.push_back ({ beat * 4 + n.s, n.len, rollRole (n.r, g), false });
            }
            std::sort (b.begin(), b.end(), [] (const Raw& a, const Raw& c) { return a.step < c.step; });
            return b; };
    }
    else
    {
        const auto& T = barTemplates (P.style);
        const int A = (int) (rr() % T.size());
        for (auto& n : T[(size_t) A]) hook.push_back ({ n.s, n.len, n.r, false });
        for (size_t k = 1; k < hook.size(); ++k) if (u (rr) < (1.f - d) * 0.3f) hook[k].len = -1; // thinning
        hook.erase (std::remove_if (hook.begin(), hook.end(), [] (const Raw& r) { return r.len < 0; }), hook.end());
        // nudge one or two hits a 16th (keeps the groove but makes it ours)
        for (int k = 0; k < 2; ++k) if (u (rr) < 0.5f && hook.size() > 2)
        {
            auto& n = hook[1 + rr() % (hook.size() - 1)]; const int ns = n.step + ((rr() & 1) ? 1 : -1);
            bool clash = ns < 1 || ns > 15; for (auto& o : hook) clash |= (&o != &n && o.step == ns);
            if (! clash) n.step = ns;
        }
        std::sort (hook.begin(), hook.end(), [] (const Raw& a, const Raw& c) { return a.step < c.step; });
        for (auto& n : hook) { n.r = rollRole (n.r, pr); n.len = rollLen (n.len, rr); }
        if (! hook.empty()) hook[0].r = 'R';
        varyBeats = [&T, &hook, &rollRole, u] (std::mt19937& g, int nBeats) {
            Bar b = hook; const auto& alt = T[g() % T.size()];
            for (int k = 0; k < nBeats; ++k)
            {
                const int beat = 1 + (int) (g() % 3);
                b.erase (std::remove_if (b.begin(), b.end(), [&] (const Raw& r) { return r.step / 4 == beat; }), b.end());
                for (auto& n : alt) if (n.s / 4 == beat) b.push_back ({ n.s, n.len, rollRole (n.r, g), false });
            }
            std::sort (b.begin(), b.end(), [] (const Raw& a, const Raw& c) { return a.step < c.step; });
            return b; };
    }
    // phrase plan: bar 2 answers with a fill, bar 3 rewrites some beats (VARIATION), bar 4 turns around.
    // 8/16 bars: the 2nd 4-bar phrase is an answer with its own variations and fills.
    std::mt19937 vr (P.rhythmSeed * 1597334677u + 3u + P.take * 2654435761u);
    const int nVary = 1 + (int) std::lround (v * 2.f);
    const Bar var3 = varyBeats (vr, nVary), var3b = varyBeats (vr, nVary), ansA = varyBeats (vr, std::max (1, nVary - 1)), ansB = varyBeats (vr, nVary + 1);
    std::mt19937& fr = P.take ? vr : rr;
    const int F1 = (int) (fr() % fills.size()), F2 = (int) (fr() % bigFills.size()), F1b = (int) (fr() % fills.size()), F2b = (int) (fr() % bigFills.size());
    for (int bar = 0; bar < P.bars; ++bar)
    {
        const int pb = bar % 4, phrase = bar / 4;
        const bool answerPhrase = phrase % 2 == 1, last = bar == P.bars - 1;
        const bool answer = (bar % 2 == 1) || P.bars == 1, turnaround = P.bars >= 4 && pb == 3;
        const Bar* src = &hook;
        if (answerPhrase) src = (pb == 2) ? &var3b : (pb == 1 ? &ansA : (pb == 3 ? &ansB : &hook));
        else if (pb == 2) src = &var3;
        else if (phrase >= 2 && pb == 1 && v > 0.3f) src = &ansA;
        const Cell* fill = nullptr;
        if (turnaround) fill = &bigFills[(size_t) ((answerPhrase || (last && P.bars > 4)) ? F2b : F2)];
        else if (answer) fill = &fills[(size_t) (answerPhrase ? F1b : F1)];
        for (auto& n : *src) if (! (fill && n.step >= 12)) raw.push_back ({ bar * 16 + n.step, n.len, n.r, false });
        if (fill) for (auto& n : *fill) raw.push_back ({ bar * 16 + 12 + n.s, n.len, n.r, true });
    }
    std::sort (raw.begin(), raw.end(), [] (const Raw& a, const Raw& b) { return a.step < b.step; });
    raw.erase (std::unique (raw.begin(), raw.end(), [] (const Raw& a, const Raw& b) { return a.step == b.step; }), raw.end());

    int shift[4]; barRoots (P, shift);
    const int nb = std::max (1, P.bars);
    auto barShift = [&] (int step) { return shift[std::min (3, (step / 16) % std::min (nb, 4))]; };

    std::vector<Note> out;
    for (size_t i = 0; i < raw.size(); ++i)
    {
        const auto& r = raw[i]; Note n; n.step = r.step;
        const int next = i + 1 < raw.size() ? raw[i + 1].step : total;
        n.len = std::max (1, std::min (r.len, next - r.step));
        char role = r.r;
        if (r.fill && m < 0.25f) role = (role == 'O' || role == 'R') ? role : 'R';
        if (! r.fill && role == 'R' && (r.step % 4) == 3 && u (pr) < m * 0.3f) role = 'O';
        if (! r.fill && role == 'O' && u (pr) < (1.f - m) * 0.4f && P.style != Bassline && P.style != SpeedGarage) role = 'R';
        n.deg = roleDeg (role) + barShift (r.step);
        const int pos = r.step % 4;
        const int base = pos == 2 ? 116 : pos == 0 ? 104 : pos == 3 ? 96 : 88;
        const float acc = 0.4f + 1.2f * P.accent;
        n.vel = std::clamp ((int) (100 + (base - 100) * acc) + (r.fill ? 4 : 0) + (int) ((u (pr) - 0.5f) * 10), 55, 127);
        out.push_back (n);
    }

    // ---------------- walk into the next chord: the last note of a bar leads to the next bar's root
    for (int bar = 0; bar < nb && nb > 1; ++bar)
    {
        const int cyc = std::min (nb, 4), nextShift = shift[std::min (3, ((bar + 1) % nb) % cyc)], thisShift = shift[std::min (3, bar % cyc)];
        if (nextShift == thisShift) continue;
        Note* last = nullptr;
        for (auto& n : out) if (n.step / 16 == bar && n.step % 16 >= 12) last = &n;
        if (last == nullptr) continue;
        // approach from a scale step away, on the side the line is coming from
        const int from = last->deg;
        int a = nextShift + (from > nextShift ? 1 : -1);
        while (a - from > 4) a -= 7;   // stay near the note before it
        while (from - a > 4) a += 7;
        last->deg = a;
        last->vel = std::min (127, last->vel + 6);
    }

    // ---------------- ghost notes: quiet pickups into the main hits
    if (P.ghosts > 0.01f && ! out.empty())
    {
        std::vector<int> taken (total, 0);
        for (auto& n : out) for (int s = n.step; s < std::min (total, n.step + n.len); ++s) taken[s] = 1;
        std::vector<Note> add;
        for (auto& n : out)
        {
            if (n.ghost) continue;
            const int s = n.step - 1;
            if (s < 0 || taken[s] || (s % 4 == 0 && P.style != Acid && P.style != Afro)) continue;
            if (u (gr) < P.ghosts * 0.75f)
            {
                Note g; g.step = s; g.len = 1; g.ghost = true;
                g.deg = (u (gr) < 0.6f) ? n.deg : barShift (s) + (n.deg >= barShift (s) + 7 ? 0 : 7);
                g.vel = std::clamp ((int) (52 + (u (gr) - 0.5f) * 12), 40, 70);
                add.push_back (g); taken[s] = 1;
            }
        }
        out.insert (out.end(), add.begin(), add.end());
        std::sort (out.begin(), out.end(), [] (const Note& a, const Note& b) { return a.step < b.step; });
        for (size_t i = 0; i + 1 < out.size(); ++i) out[i].len = std::max (1, std::min (out[i].len, out[i + 1].step - out[i].step));
    }

    // ---------------- slides
    const float sp = std::max (slideAmount (P.style), 0.08f * m);
    for (size_t i = 1; i < out.size(); ++i)
    {
        auto& n = out[i]; const auto& p = out[i - 1];
        const bool leg = p.step + std::max (p.len, 1) >= n.step && n.deg != p.deg && std::abs (n.deg - p.deg) <= 7 && ! n.ghost && ! p.ghost;
        n.slide = leg && u (pr) < sp;
    }
    for (size_t i = 0; i + 1 < out.size(); ++i) if (out[i + 1].slide) out[i].len = out[i + 1].step - out[i].step;

    // ---------------- mutations (MUTATE button)
    if (P.mutations > 0 && ! out.empty())
    {
        std::mt19937 mr (P.mutateSeed);
        static const char roles[] = { 'R', 'O', '5', '7', '3' };
        int done = 0;
        for (int tries = 0; done < P.mutations * 2 && tries < 200; ++tries)
        {
            auto& n = out[mr() % out.size()];
            if (n.ghost) continue;
            if (mr() % 3 == 0) { n.vel = std::clamp (n.vel + 10 + (int) (mr() % 21) * ((mr() & 1) ? 1 : -1), 55, 127); ++done; }
            else
            {
                const int nd = roleDeg (roles[mr() % 5]) + barShift (n.step);
                if (nd != n.deg) { n.deg = nd; ++done; }
            }
        }
    }
    return out;
}
} // namespace ut
