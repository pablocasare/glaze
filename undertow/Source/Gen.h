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

// ---------------------------------------------------------------- building blocks
// A cell is a few notes; role picks the pitch: R root, O octave, 5 fifth, 7 b7 below, 3 third, 4 fourth, 6 b6 below, 2 second
struct Cn { int s, len; char r; };
using Cell = std::vector<Cn>;
inline int roleDeg (char r)
{
    switch (r) { case 'O': return 7; case '5': return 4; case '7': return -1; case '3': return 2; case '4': return 3; case '6': return -2; case '2': return 1; default: return 0; }
}
// one-beat cells (offsets 0..3 inside the beat, 0 = on the kick)
inline const std::vector<Cell>& beatCells (int style)
{
    static const std::vector<Cell> rolling { { { 2, 2, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } },
                                             { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> offbeat { { { 2, 2, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } } };
    static const std::vector<Cell> bouncy { { { 2, 1, 'O' } }, { { 2, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'O' }, { 2, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> driving { { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> acid { { { 0, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 0, 2, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 2, 'O' } },
                                          { { 0, 1, 'R' }, { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 0, 1, 'R' }, { 1, 1, 'O' }, { 2, 1, 'R' }, { 3, 1, '5' } } };
    switch (style) { case Offbeat: return offbeat; case Bouncy: return bouncy; case Driving: return driving; case Acid: return acid; default: return rolling; }
}
// last-beat fills: where the line moves
inline const std::vector<Cell>& fillCells (int style)
{
    static const std::vector<Cell> f { { { 2, 1, 'R' }, { 3, 1, '7' } }, { { 1, 1, 'R' }, { 2, 1, '5' }, { 3, 1, 'O' } }, { { 2, 1, '7' }, { 3, 1, 'R' } },
                                       { { 2, 1, '3' }, { 3, 1, '4' } }, { { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } }, { { 2, 1, '5' }, { 3, 1, '7' } } };
    static const std::vector<Cell> off { { { 2, 1, 'R' }, { 3, 1, '5' } }, { { 2, 2, '7' } }, { { 2, 2, '5' } }, { { 2, 1, 'O' }, { 3, 1, '7' } } };
    static const std::vector<Cell> acid { { { 0, 1, 'O' }, { 1, 1, '7' }, { 2, 1, '5' }, { 3, 1, '3' } }, { { 0, 1, 'R' }, { 1, 1, '3' }, { 2, 1, '4' }, { 3, 1, '5' } }, { { 0, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, '7' } } };
    if (style == Offbeat || style == Minimal) return off;
    if (style == Acid) return acid;
    return f;
}
// whole-bar templates for the styles that don't think in beats
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
    return style == TwoStep ? twoStep : (style == Minimal ? minimal : broken);
}
inline bool usesBeatCells (int style) { return style != TwoStep && style != Minimal && style != Broken; }

inline std::vector<Note> generate (const Params& P)
{
    std::mt19937 rr (P.rhythmSeed * 2654435761u + (uint32_t) P.style * 97u), pr (P.pitchSeed * 40503u + 7u);
    auto u = [] (std::mt19937& g) { return std::uniform_real_distribution<float> (0.f, 1.f) (g); };
    auto pickIdx = [&] (int n, float centre) { // around centre (0..1), a little random
        const float x = std::clamp (centre * (n - 1) + (u (rr) - 0.5f) * 1.6f, 0.f, (float) (n - 1)); return (int) std::lround (x); };
    const float m = P.movement, v = P.variation, d = P.density;

    struct Raw { int step, len; char r; bool fill; };
    std::vector<Raw> raw;
    if (usesBeatCells (P.style))
    {
        const auto& cells = beatCells (P.style); const auto& fills = fillCells (P.style);
        // sort by how busy they are so density means something
        std::vector<int> order (cells.size()); for (size_t i = 0; i < order.size(); ++i) order[i] = (int) i;
        std::stable_sort (order.begin(), order.end(), [&] (int a, int b) { return cells[(size_t) a].size() < cells[(size_t) b].size(); });
        const int A = order[(size_t) pickIdx ((int) cells.size(), d)];
        int B = order[(size_t) pickIdx ((int) cells.size(), std::min (1.f, d + 0.2f))]; if (B == A) B = order[(size_t) ((std::find (order.begin(), order.end(), A) - order.begin() + 1) % (int) order.size())];
        const int F1 = (int) (rr() % fills.size()), F2 = (int) (rr() % fills.size());
        for (int bar = 0; bar < P.bars; ++bar)
            for (int beat = 0; beat < 4; ++beat)
            {
                const bool answer = (bar % 2 == 1) || P.bars == 1;
                const Cell* c = &cells[(size_t) A]; bool fill = false;
                if (beat == 3 && answer) { c = &fills[(size_t) (bar == 3 ? F2 : F1)]; fill = true; }
                else if (beat == 3 && u (rr) < 0.25f + v * 0.5f) c = &cells[(size_t) B];
                else if (beat == 1 && u (rr) < v * 0.45f) c = &cells[(size_t) B];
                for (auto& n : *c) raw.push_back ({ bar * 16 + beat * 4 + n.s, n.len, n.r, fill });
            }
    }
    else
    {
        const auto& T = barTemplates (P.style);
        std::vector<int> order (T.size()); for (size_t i = 0; i < order.size(); ++i) order[i] = (int) i;
        std::stable_sort (order.begin(), order.end(), [&] (int a, int b) { return T[(size_t) a].size() < T[(size_t) b].size(); });
        const int A = order[(size_t) pickIdx ((int) T.size(), d)];
        const auto& fills = fillCells (P.style); const int F = (int) (rr() % fills.size());
        for (int bar = 0; bar < P.bars; ++bar)
        {
            const bool answer = (bar % 2 == 1) || P.bars == 1;
            for (auto& n : T[(size_t) A])
            {
                if (answer && n.s >= 12 && u (rr) < 0.5f + v * 0.5f) continue; // replaced by the fill
                if (n.s != T[(size_t) A][0].s && u (rr) < (1.f - d) * 0.25f) continue;   // thin it out at low density
                raw.push_back ({ bar * 16 + n.s, n.len, n.r, false });
            }
            if (answer && (raw.empty() || raw.back().step % 16 < 12))
                for (auto& n : fills[(size_t) F]) raw.push_back ({ bar * 16 + 12 + n.s, n.len, n.r, true });
            if (d > 0.7f && P.style != TwoStep) // ghost offbeat
                if (u (rr) < (d - 0.7f) * 2.5f) raw.push_back ({ bar * 16 + 6, 1, 'R', false });
        }
        std::sort (raw.begin(), raw.end(), [] (const Raw& a, const Raw& b) { return a.step < b.step; });
        raw.erase (std::unique (raw.begin(), raw.end(), [] (const Raw& a, const Raw& b) { return a.step == b.step; }), raw.end());
    }

    // ---------------- pitch
    // harmonic move: the answer bar can shift the whole line to VI, VII or iv (only with enough movement)
    int shift[4] = { 0, 0, 0, 0 };
    if (P.bars >= 2 && u (pr) < (m - 0.45f) * 1.6f)
    {
        static const int moves[] = { -2, -1, 3 };
        const int mv = moves[pr() % 3];
        if (P.bars == 2) shift[1] = mv; else { shift[2] = mv; shift[3] = (u (pr) < 0.5f ? mv : 0); }
    }
    std::vector<Note> out;
    const int total = P.bars * 16;
    for (size_t i = 0; i < raw.size(); ++i)
    {
        const auto& r = raw[i]; Note n; n.step = r.step;
        const int next = i + 1 < raw.size() ? raw[i + 1].step : total;
        n.len = std::max (1, std::min (r.len, next - r.step));
        char role = r.r;
        if (r.fill && m < 0.25f) role = (role == 'O' || role == 'R') ? role : 'R';     // low movement: fills stay home
        if (! r.fill && role == 'R' && (r.step % 4) == 3 && u (pr) < m * 0.3f) role = 'O'; // octave pops on the "a"
        if (! r.fill && role == 'O' && u (pr) < (1.f - m) * 0.4f) role = 'R';
        n.deg = roleDeg (role) + (role == 'O' || r.fill ? 0 : 0) + shift[std::min (3, r.step / 16)];
        if (shift[std::min (3, r.step / 16)] != 0 && role == 'O') n.deg = 7 + shift[std::min (3, r.step / 16)];
        const int pos = r.step % 4;
        n.vel = std::clamp ((pos == 2 ? 116 : pos == 0 ? 104 : pos == 3 ? 96 : 88) + (r.fill ? 4 : 0) + (int) ((u (pr) - 0.5f) * 12), 60, 127);
        const bool leg = ! out.empty() && out.back().step + out.back().len >= n.step && n.deg != out.back().deg;
        const float sp = P.style == Acid ? 0.55f : (P.style == TwoStep ? 0.35f : 0.08f * m);
        n.slide = leg && u (pr) < sp;
        out.push_back (n);
    }
    for (size_t i = 0; i + 1 < out.size(); ++i) if (out[i + 1].slide) out[i].len = out[i + 1].step - out[i].step;

    // ---------------- mutations (MUTATE button)
    if (P.mutations > 0 && ! out.empty())
    {
        std::mt19937 mr (P.mutateSeed);
        static const char roles[] = { 'R', 'O', '5', '7', '3' };
        for (int k = 0; k < P.mutations * 2; ++k)
        {
            auto& n = out[mr() % out.size()];
            if (mr() % 3 == 0) n.vel = std::clamp (n.vel + (int) (mr() % 31) - 15, 60, 127);
            else n.deg = roleDeg (roles[mr() % 5]);
        }
    }
    return out;
}
} // namespace ut
