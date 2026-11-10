#pragma once
// Undertow bassline generator. Pure and deterministic: same params + seeds = same pattern.
//
// A line is a hook, not a fresh roll of the dice every bar:
//   variation 0.00–0.20  A A A A'   the hook repeats, a fill only on the last bar of each 4
//   variation 0.20–0.60  A B A B'   two-bar call and response
//   variation 0.60–1.00  A B A' C   the hook, an answer, a rewrite, a turnaround
// Ghosts, slides and the hook's pitches are part of the bar, so a repeated bar is the same bar.
// EVOLVE (Params::take) only changes the answer and the fill, never the hook.
#include <vector>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstdint>

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
    static const char* n[] = {
        "16ths rolling between the kicks",
        "short notes, octave jumps",
        "classic house offbeat, on the and",
        "swung UKG, long notes and slides",
        "sparse hook that loops",
        "16ths between the kicks",
        "syncopated against a broken kick",
        "303 slides, accents and rests",
        "held octave wobble for a Reese",
        "tresillo 3-3-2, the 5th answers",
        "long offbeat subs that slide",
        "UK bassline, bouncy offbeat octaves" };
    return n[s];
}

// Suggested groove when the style is chosen. The host still owns the real tempo.
inline float styleSwing (int s)
{
    switch (s)
    {
        case TwoStep: return 0.60f;
        case SpeedGarage: return 0.59f;
        case Afro: return 0.55f;
        case Bassline: return 0.55f;
        case Bouncy: return 0.54f;
        case Offbeat: return 0.54f;
        case DeepTech: return 0.54f;
        case Acid: return 0.54f;
        case Broken: return 0.54f;
        case Rolling: return 0.53f;
        case Minimal: return 0.53f;
        case Driving: return 0.51f;
        default: return 0.54f;
    }
}
inline int styleBpm (int s)
{
    switch (s)
    {
        case Bassline: return 138;
        case SpeedGarage: return 136;
        case TwoStep: return 132;
        case Driving: return 128;
        case Rolling: return 126;
        case Bouncy: return 126;
        case Broken: return 126;
        case Acid: return 126;
        case Offbeat: return 124;
        case Minimal: return 125;
        case DeepTech: return 125;
        case Afro: return 123;
        default: return 126;
    }
}

// 4/4 except the styles whose kick is the point of the rhythm.
inline bool isKickStep (int style, int step16)
{
    step16 &= 15;
    if (style == TwoStep) return step16 == 0 || step16 == 7 || step16 == 8 || step16 == 15;
    if (style == Broken) return step16 == 0 || step16 == 6 || step16 == 9 || step16 == 13;
    return (step16 & 3) == 0;
}

// Where a 16th sits, in quarter-note beats. Odd 16ths take the MPC swing; the house "&" sits a few ms early.
inline double noteOnBeat (int step, float swing, int style)
{
    double b = step * 0.25 + ((step & 1) ? ((double) swing - 0.5) * 0.5 : 0.0);
    if ((step % 4) == 2 && (style == Offbeat || style == DeepTech || style == Rolling || style == Minimal))
        b -= 0.010;
    return b;
}
inline double noteOffBeat (int step, int len, float swing, float gate, int style, bool legato)
{
    const double on = noteOnBeat (step, swing, style);
    if (legato) return noteOnBeat (step + len, swing, style) + 0.02;
    const float gk = 0.28f + 0.70f * std::max (0.f, std::min (1.f, gate));
    const double full = noteOnBeat (step + std::max (1, len), swing, style) - on;
    return on + std::max (0.05, full * (double) gk - 0.008);
}

enum Scale { Minor, Dorian, Phrygian, HarmMinor, Major, MinorPent, Mixolydian, NumScales };
inline const char* scaleName (int s)
{
    static const char* n[] = { "Minor", "Dorian", "Phrygian", "Harm. Minor", "Major", "Minor Pent.", "Mixolydian" };
    return n[s];
}
inline const int* scaleSteps (int s)
{
    static const int sc[][7] = {
        { 0, 2, 3, 5, 7, 8, 10 },
        { 0, 2, 3, 5, 7, 9, 10 },
        { 0, 1, 3, 5, 7, 8, 10 },
        { 0, 2, 3, 5, 7, 8, 11 },
        { 0, 2, 4, 5, 7, 9, 11 },
        { 0, 3, 5, 7, 7, 10, 10 }, // minor pentatonic; repeated degrees stay inside the five notes
        { 0, 2, 4, 5, 7, 9, 10 } };
    return sc[std::max (0, std::min ((int) Mixolydian, s))];
}
inline int degToSemi (int deg, int scale)
{
    const int o = (int) std::floor (deg / 7.0);
    const int d = deg - o * 7;
    return scaleSteps (scale)[d] + 12 * o;
}

// chord progressions as the bass root's scale degree per bar (VI and VII sit below the root)
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
    uint32_t rhythmSeed = 1, pitchSeed = 1, mutateSeed = 0, take = 0;
    int mutations = 0;
};

// ---------------------------------------------------------------- building blocks
// A cell is one beat. Role picks the pitch: R root, O octave, 5 fifth, 7 b7 below, 3 third, 4 fourth, 6 b6, 2 second.
struct Cn { int s, len; char r; };
using Cell = std::vector<Cn>;
inline int roleDeg (char r)
{
    switch (r) { case 'O': return 7; case '5': return 4; case '7': return -1; case '3': return 2; case '4': return 3; case '6': return -2; case '2': return 1; default: return 0; }
}
inline bool usesBeatCells (int style)
{
    return style == Rolling || style == Bouncy || style == Offbeat || style == Driving || style == Acid || style == Bassline;
}

inline const std::vector<Cell>& beatCells (int style)
{
    static const std::vector<Cell> rolling { { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } } };
    static const std::vector<Cell> offbeat { { { 2, 2, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } } };
    static const std::vector<Cell> bouncy { { { 2, 1, 'O' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'O' }, { 2, 1, 'R' } } };
    static const std::vector<Cell> driving { { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 2, 1, 'R' } } };
    static const std::vector<Cell> acid { { { 0, 1, 'R' }, { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 0, 2, 'R' }, { 2, 1, 'O' }, { 3, 1, '5' } } };
    static const std::vector<Cell> bassline { { { 2, 1, 'O' } }, { { 2, 1, 'O' }, { 3, 1, 'R' } } };
    switch (style)
    {
        case Offbeat: return offbeat; case Bouncy: return bouncy; case Driving: return driving; case Acid: return acid;
        case Bassline: return bassline; default: return rolling;
    }
}
inline const std::vector<Cell>& barTemplates (int style)
{
    static const std::vector<Cell> twoStep { { { 0, 4, 'R' }, { 6, 2, '5' }, { 8, 4, 'O' }, { 14, 2, '7' } },
                                              { { 0, 3, 'R' }, { 4, 2, 'O' }, { 8, 3, 'R' }, { 12, 2, '5' }, { 14, 2, '7' } } };
    static const std::vector<Cell> minimal { { { 2, 2, 'R' }, { 6, 2, 'O' }, { 10, 2, 'R' } }, { { 2, 3, 'R' }, { 10, 3, 'R' }, { 14, 1, '7' } } };
    static const std::vector<Cell> broken { { { 2, 2, 'R' }, { 4, 2, 'O' }, { 8, 1, 'R' }, { 11, 2, '5' }, { 14, 2, 'R' } } };
    static const std::vector<Cell> afro { { { 0, 2, 'R' }, { 3, 2, 'R' }, { 6, 2, '5' }, { 8, 2, 'R' }, { 11, 2, '5' }, { 14, 2, 'R' } } };
    static const std::vector<Cell> deep { { { 2, 4, 'R' }, { 10, 4, 'R' }, { 14, 2, '7' } }, { { 2, 6, 'R' }, { 10, 4, 'O' }, { 14, 2, 'R' } } };
    static const std::vector<Cell> speed { { { 2, 4, 'O' }, { 6, 4, 'R' }, { 10, 4, 'O' }, { 14, 2, 'R' } }, { { 2, 6, 'O' }, { 10, 4, 'R' }, { 14, 2, 'O' } } };
    switch (style)
    {
        case TwoStep: return twoStep; case Minimal: return minimal; case Afro: return afro; case DeepTech: return deep;
        case SpeedGarage: return speed; default: return broken;
    }
}

inline uint32_t mix32 (uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16; return x;
}
inline float hash01 (uint32_t x) { return (mix32 (x) & 0xffffff) / float (0x1000000); }

inline void barRoots (const Params& P, int* shift)
{
    for (int b = 0; b < 4; ++b) shift[b] = 0;
    if (P.bars == 1) return;
    if (P.chords == ChordAuto)
    {
        std::mt19937 hr (P.pitchSeed * 2246822519u + 13u);
        float chance = (P.movement - 0.15f) * 1.15f;
        if (P.style == Minimal || P.style == DeepTech) chance *= 0.35f; // hypnotic styles stay home unless Movement is high
        if (std::uniform_real_distribution<float> (0.f, 1.f) (hr) < chance)
        {
            static const int prog[] = { ChordI_VI, ChordI_VII, ChordI_iv, ChordI_VI_VII };
            const int c = prog[hr() % 4];
            for (int b = 0; b < 4; ++b) shift[b] = chordRoots (c)[b];
        }
    }
    else for (int b = 0; b < 4; ++b) shift[b] = chordRoots (P.chords)[b];
    if (P.bars == 2 && P.chords == ChordI_VI_VII) shift[1] = -2;
}

namespace detail
{
struct Hn { int step, len; char r; };
using Bar = std::vector<Hn>;

inline void sortBar (Bar& b)
{
    std::sort (b.begin(), b.end(), [] (const Hn& a, const Hn& c) { return a.step < c.step; });
    b.erase (std::unique (b.begin(), b.end(), [] (const Hn& a, const Hn& c) { return a.step == c.step; }), b.end());
    for (size_t i = 0; i + 1 < b.size(); ++i)
        b[i].len = std::max (1, std::min (b[i].len, b[i + 1].step - b[i].step));
    if (! b.empty()) b.back().len = std::max (1, std::min (b.back().len, 16 - b.back().step));
}
inline void add (Bar& b, int step, int len, char r)
{
    if (step < 0 || step > 15 || len < 1) return;
    for (auto& n : b) if (n.step == step) return;
    b.push_back ({ step, std::min (len, 16 - step), r });
}
inline float u01 (std::mt19937& g) { return std::uniform_real_distribution<float> (0.f, 1.f) (g); }

inline char rollRole (char r, float m, bool octavey, std::mt19937& g)
{
    // Movement 0 keeps the written pitch. Higher movement swaps in a neighbouring chord tone.
    if (m < 0.02f || u01 (g) > m * 0.80f) return r;
    const float w[] = { 1.0f, octavey ? 1.05f : 0.30f, 0.50f * m + 0.04f, 0.42f * m + 0.04f, 0.22f * m, 0.16f * m, 0.08f * m, 0.08f * m };
    static const char rs[] = { 'R', 'O', '5', '7', '3', '4', '6', '2' };
    float t = 0; for (float x : w) t += x;
    float x = u01 (g) * t;
    for (int i = 0; i < 8; ++i) { x -= w[i]; if (x <= 0) return rs[i]; }
    return 'R';
}

inline bool longStyle (int s)
{
    return s == TwoStep || s == SpeedGarage || s == DeepTech || s == Minimal || s == Afro;
}

inline Bar buildHook (const Params& P, uint32_t salt)
{
    std::mt19937 rr (P.rhythmSeed * 2654435761u + (uint32_t) P.style * 97u + salt * 131u);
    std::mt19937 pr (P.pitchSeed * 40503u + 7u + salt * 17u);
    const float d = std::max (0.f, std::min (1.f, P.density));
    const float m = std::max (0.f, std::min (1.f, P.movement));
    Bar b;

    switch (P.style)
    {
        case Offbeat:
            for (int beat = 0; beat < 4; ++beat)
            {
                const bool pickup = d > 0.60f && u01 (rr) < (d - 0.60f) * 2.4f;
                if (pickup) { add (b, beat * 4 + 2, 1, 'R'); add (b, beat * 4 + 3, 1, u01 (pr) < m * 0.7f ? '5' : 'R'); }
                else add (b, beat * 4 + 2, 2, 'R');
            }
            break;

        case Driving:
        {
            const int tier = d < 0.30f ? 1 : d < 0.55f ? 2 : 3; // &  →  &+a  →  e+&+a
            for (int beat = 0; beat < 4; ++beat)
            {
                if (tier >= 3) add (b, beat * 4 + 1, 1, 'R');
                add (b, beat * 4 + 2, 1, u01 (pr) < m * 0.5f ? 'O' : 'R');
                if (tier >= 2) add (b, beat * 4 + 3, 1, 'R');
            }
            break;
        }

        case Bassline:
            for (int beat = 0; beat < 4; ++beat)
            {
                add (b, beat * 4 + 2, 1, 'O');
                if (d > 0.72f && u01 (rr) < (d - 0.72f) * 2.6f) add (b, beat * 4 + 3, 1, (beat & 1) ? 'R' : 'O');
                if (d > 0.88f && u01 (rr) < (d - 0.88f) * 4.f) add (b, beat * 4 + 1, 1, 'R');
            }
            break;

        case Rolling:
        case Bouncy:
        {
            const bool oct = P.style == Bouncy;
            std::vector<Bar> cells;
            if (oct)
            {
                cells = { { { 2, 1, 'O' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'O' }, { 2, 1, 'R' } },
                          { { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
            }
            else
            {
                cells = { { { 2, 2, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 3, 1, 'R' } },
                          { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'R' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' } } };
            }
            const float target = oct ? (1.05f + d * 1.6f) : (1.15f + d * 1.75f);
            auto choose = [&] {
                float t = 0; std::vector<float> w;
                for (auto& c : cells) { const float ww = 1.f / (0.35f + std::fabs ((float) c.size() - target)); w.push_back (ww); t += ww; }
                float x = u01 (rr) * t;
                for (size_t i = 0; i < w.size(); ++i) { x -= w[i]; if (x <= 0) return (int) i; }
                return 0;
            };
            int X = choose(), Y = choose(), Z = choose();
            for (int k = 0; Y == X && k < 5; ++k) Y = choose();
            static const int forms[][4] = { { 0, 0, 0, 1 }, { 0, 1, 0, 1 }, { 0, 0, 1, 0 }, { 0, 1, 0, 2 }, { 0, 0, 0, 0 } };
            const int form = (int) (rr() % (d < 0.35f ? 5 : 4));
            const int ids[] = { X, Y, Z };
            for (int beat = 0; beat < 4; ++beat)
                for (auto& n : cells[(size_t) ids[forms[form][beat]]]) add (b, beat * 4 + n.step, n.len, n.r);
            break;
        }

        case Acid:
        {
            static const Bar pats[] = {
                { { 0, 2, 'R' }, { 2, 1, 'R' }, { 3, 1, 'O' }, { 5, 1, 'R' }, { 6, 2, 'O' }, { 8, 1, 'R' }, { 9, 1, 'R' }, { 10, 1, '5' }, { 12, 1, 'R' }, { 14, 2, 'O' } },
                { { 0, 1, 'R' }, { 1, 1, 'R' }, { 2, 2, 'O' }, { 5, 1, 'R' }, { 6, 1, 'R' }, { 8, 1, 'O' }, { 10, 2, 'R' }, { 12, 1, '5' }, { 13, 1, '7' }, { 14, 1, 'R' }, { 15, 1, 'O' } },
                { { 0, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, 'R' }, { 4, 2, 'R' }, { 7, 1, '5' }, { 8, 1, 'R' }, { 10, 1, 'O' }, { 11, 1, 'R' }, { 13, 1, '3' }, { 14, 1, '4' }, { 15, 1, '5' } },
                { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 2, 'O' }, { 6, 1, 'R' }, { 7, 1, '5' }, { 8, 2, 'R' }, { 11, 1, 'O' }, { 12, 1, 'R' }, { 14, 2, '7' } },
                { { 0, 1, 'O' }, { 1, 1, 'R' }, { 3, 1, 'R' }, { 4, 1, '5' }, { 6, 1, 'R' }, { 8, 1, 'O' }, { 9, 1, 'R' }, { 11, 1, '5' }, { 12, 2, 'R' }, { 15, 1, '7' } },
                { { 0, 2, 'R' }, { 3, 1, 'O' }, { 4, 1, 'R' }, { 6, 2, '5' }, { 8, 1, 'R' }, { 9, 1, 'O' }, { 10, 1, 'R' }, { 12, 1, '7' }, { 13, 1, '5' }, { 14, 1, '4' }, { 15, 1, 'R' } } };
            b = pats[rr() % 6];
            if (d < 0.45f)
            {
                Bar kept; int dropped = 0;
                for (auto& n : b)
                {
                    const bool weak = (n.step % 4) == 1 || (n.step % 4) == 3;
                    if (weak && dropped < 3 && u01 (rr) < (0.45f - d) * 2.2f) { ++dropped; continue; }
                    kept.push_back (n);
                }
                if ((int) kept.size() >= 6) b.swap (kept);
            }
            break;
        }

        case SpeedGarage:
        {
            static const Bar pats[] = {
                { { 2, 4, 'O' }, { 6, 4, 'R' }, { 10, 4, 'O' }, { 14, 2, 'R' } },
                { { 2, 3, 'O' }, { 6, 3, 'R' }, { 10, 3, 'O' }, { 14, 2, '7' } },
                { { 2, 6, 'O' }, { 10, 4, 'R' }, { 14, 2, 'O' } },
                { { 2, 4, 'R' }, { 6, 4, 'O' }, { 10, 4, 'R' }, { 14, 2, '7' } },
                { { 2, 4, 'O' }, { 6, 2, 'R' }, { 10, 4, 'O' }, { 14, 2, 'R' } } };
            b = pats[d < 0.28f ? (2 + (int) (rr() % 2)) : (int) (rr() % 5)];
            if (d > 0.80f)
            {
                Bar extra;
                for (auto& n : b)
                {
                    const int flick = n.step + n.len - 1; // last 16th of the hold, never a kick onset
                    if (n.len >= 4 && flick > n.step && flick < 16 && ! isKickStep (SpeedGarage, flick) && u01 (rr) < (d - 0.75f) * 2.f)
                    {
                        extra.push_back ({ n.step, flick - n.step, n.r });
                        extra.push_back ({ flick, 1, n.r == 'O' ? 'R' : 'O' });
                    }
                    else extra.push_back (n);
                }
                b.swap (extra);
            }
            break;
        }

        case TwoStep:
        {
            static const Bar pats[] = {
                { { 0, 4, 'R' }, { 6, 2, '5' }, { 8, 4, 'O' }, { 14, 2, '7' } },
                { { 0, 3, 'R' }, { 4, 2, 'O' }, { 8, 3, 'R' }, { 12, 2, '5' }, { 14, 2, '7' } },
                { { 2, 3, 'R' }, { 6, 2, 'O' }, { 10, 4, 'R' }, { 14, 2, '7' } },
                { { 0, 2, 'R' }, { 3, 3, 'O' }, { 8, 4, 'R' }, { 13, 3, '5' } },
                { { 0, 4, 'R' }, { 8, 4, 'O' }, { 12, 4, '5' } } };
            b = pats[rr() % 5];
            if (d < 0.35f && b.size() > 4) b.pop_back();
            if (d > 0.78f)
            {
                for (int s = 1; s < 15; ++s)
                {
                    bool taken = false; for (auto& n : b) taken |= n.step == s || (s > n.step && s < n.step + n.len);
                    if (! taken && u01 (rr) < (d - 0.78f) * 1.5f) { add (b, s, 1, u01 (pr) < 0.5f ? 'R' : '5'); break; }
                }
            }
            break;
        }

        case Minimal:
        {
            static const Bar pats[] = {
                { { 2, 2, 'R' }, { 6, 2, 'O' }, { 10, 2, 'R' } },
                { { 2, 3, 'R' }, { 10, 3, 'R' }, { 14, 1, '7' } },
                { { 2, 2, 'R' }, { 10, 2, 'R' }, { 14, 2, '5' } },
                { { 2, 2, 'R' }, { 6, 2, 'O' }, { 14, 2, 'R' } },
                { { 6, 3, 'R' }, { 10, 2, 'R' }, { 14, 2, '7' } } };
            b = pats[rr() % 5];
            if (d > 0.75f)
            {
                for (int s : { 14, 6, 15, 3 })
                {
                    bool taken = false; for (auto& n : b) taken |= n.step == s;
                    if (! taken) { add (b, s, 1, 'R'); break; }
                }
            }
            break;
        }

        case Broken:
        {
            static const Bar pats[] = {
                { { 2, 2, 'R' }, { 4, 2, 'O' }, { 8, 1, 'R' }, { 11, 2, '5' }, { 14, 2, 'R' } },
                { { 1, 2, 'R' }, { 4, 1, 'O' }, { 7, 2, 'R' }, { 11, 2, '5' }, { 15, 1, '7' } },
                { { 3, 2, 'R' }, { 8, 2, 'O' }, { 11, 1, 'R' }, { 14, 2, '5' } },
                { { 2, 2, 'R' }, { 4, 2, '5' }, { 8, 2, 'R' }, { 11, 2, 'O' }, { 15, 1, '7' } },
                { { 1, 1, 'R' }, { 3, 2, 'O' }, { 8, 1, 'R' }, { 10, 2, '5' }, { 14, 2, 'R' } } };
            b = pats[rr() % 5];
            if (d < 0.30f && b.size() > 4) b.pop_back();
            break;
        }

        case Afro:
        {
            static const Bar pats[] = {
                { { 0, 2, 'R' }, { 3, 2, 'R' }, { 6, 2, '5' }, { 8, 2, 'R' }, { 11, 2, '5' }, { 14, 2, 'R' } },
                { { 0, 2, 'R' }, { 3, 2, '5' }, { 6, 2, 'R' }, { 8, 2, 'O' }, { 11, 2, 'R' }, { 14, 2, '7' } },
                { { 0, 3, 'R' }, { 3, 3, '5' }, { 6, 2, 'R' }, { 8, 2, 'R' }, { 11, 3, 'O' }, { 14, 2, 'R' } },
                { { 0, 2, 'R' }, { 3, 2, 'R' }, { 6, 2, '5' }, { 8, 2, '5' }, { 11, 2, 'R' }, { 14, 2, '7' } } };
            b = pats[rr() % 4];
            if (d < 0.32f)
                b.erase (std::remove_if (b.begin(), b.end(), [] (const Hn& n) { return n.step == 8 || n.step == 11; }), b.end());
            break;
        }

        case DeepTech:
        {
            static const Bar pats[] = {
                { { 2, 4, 'R' }, { 10, 4, 'R' }, { 14, 2, '7' } },
                { { 2, 6, 'R' }, { 10, 4, 'O' }, { 14, 2, 'R' } },
                { { 2, 3, 'R' }, { 6, 2, '5' }, { 10, 4, 'R' } },
                { { 6, 4, 'R' }, { 10, 3, 'R' }, { 14, 2, '7' } },
                { { 2, 6, 'R' }, { 10, 4, 'R' }, { 14, 2, '5' } } };
            b = pats[rr() % 5];
            if (d < 0.28f && b.size() > 3) b.pop_back();
            break;
        }

        default: break;
    }

    const bool octavey = P.style == Bouncy || P.style == Bassline || P.style == SpeedGarage || P.style == Acid;
    const float mEff = m * (P.style == Minimal || P.style == DeepTech || P.style == Offbeat ? 0.45f
                            : P.style == Driving || P.style == Afro ? 0.65f
                            : P.style == Bassline || P.style == SpeedGarage ? 0.35f : 1.f);
    if (P.style != Bassline && P.style != SpeedGarage && P.style != Afro)
        for (auto& n : b) n.r = rollRole (n.r, mEff, octavey, pr);
    else if (m > 0.15f)
        for (size_t i = 1; i < b.size(); ++i) if (u01 (pr) < mEff * 0.45f) b[i].r = rollRole (b[i].r, mEff, octavey, pr);

    sortBar (b);
    if (! b.empty() && P.style != Bassline && P.style != SpeedGarage && P.style != Bouncy) b.front().r = 'R';
    if (P.style == Afro)
        for (auto& n : b) if (n.step == 0) n.r = 'R'; // the tresillo lands on the root
    return b;
}

inline Bar fill (int style, bool big, int index)
{
    static const Bar small[] = {
        { { 1, 1, '5' }, { 2, 1, '7' }, { 3, 1, 'R' } },
        { { 2, 1, '7' }, { 3, 1, '5' } },
        { { 2, 2, '5' } },
        { { 1, 1, 'R' }, { 2, 1, '5' }, { 3, 1, '7' } },
        { { 2, 1, 'O' }, { 3, 1, '7' } } };
    static const Bar large[] = {
        { { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } },
        { { 1, 1, '5' }, { 2, 1, '4' }, { 3, 1, '7' } },
        { { 1, 1, '3' }, { 2, 1, '5' }, { 3, 1, '7' } },
        { { 2, 1, '7' }, { 3, 1, 'O' } } };
    static const Bar acidS[] = {
        { { 0, 1, 'O' }, { 1, 1, '7' }, { 2, 1, '5' }, { 3, 1, '3' } },
        { { 0, 1, 'R' }, { 1, 1, '3' }, { 2, 1, '4' }, { 3, 1, '5' } },
        { { 1, 1, 'O' }, { 2, 1, '5' }, { 3, 1, '7' } } };
    static const Bar acidB[] = {
        { { 0, 1, 'O' }, { 1, 1, '7' }, { 2, 1, '5' }, { 3, 1, '4' } },
        { { 0, 1, '5' }, { 1, 1, '4' }, { 2, 1, '3' }, { 3, 1, 'R' } },
        { { 0, 1, 'R' }, { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } } };
    if (style == Acid) return (big ? acidB[index % 3] : acidS[index % 3]);
    return big ? large[index % 4] : small[index % 5];
}

enum Role { Hook, Answer, Vary, Turn };
inline Role roleOf (int bar, int nbars, float v)
{
    if (nbars <= 1) return v > 0.65f ? Answer : Hook;
    const int pb = bar % 4;
    if (v <= 0.20f) return (pb == 3 || bar == nbars - 1) ? Turn : Hook;
    if (v <= 0.60f)
    {
        if (pb == 1) return Answer;
        if (pb == 3) return Turn;
        return Hook;
    }
    if (pb == 1) return Answer;
    if (pb == 2) return Vary;
    if (pb == 3) return Turn;
    return Hook;
}

inline void paintPitch (Bar& b, Role role, int phrase, uint32_t take)
{
    if (role == Hook || b.empty()) return;
    int idx = role == Answer || role == Vary ? (int) b.size() / 2 : -1;
    if (role == Turn)
    {
        for (int i = (int) b.size() - 1; i >= 0; --i) if (b[(size_t) i].step >= 12) { idx = i; break; }
        if (idx < 0) { b.push_back ({ 14, 2, '7' }); sortBar (b); idx = (int) b.size() - 1; }
    }
    if (idx < 0 || idx >= (int) b.size()) return;
    static const char choices[] = { '7', '5', '3', 'O' };
    char next = choices[(take + (uint32_t) phrase + (uint32_t) role) % 4];
    if (next == b[(size_t) idx].r) next = next == '5' ? '7' : '5';
    b[(size_t) idx].r = next;
}

inline void paintFill (Bar& b, const Bar& f, uint32_t take)
{
    for (auto& n : b) if (n.step < 12 && n.step + n.len > 12) n.len = std::max (1, 12 - n.step);
    b.erase (std::remove_if (b.begin(), b.end(), [] (const Hn& n) { return n.step >= 12; }), b.end());
    for (auto n : f) { n.step += 12; if (n.step < 16) b.push_back (n); }
    sortBar (b);
    if (! b.empty())
    {
        static const char o[] = { '7', '5', '3' };
        b.back().r = o[take % 3];
    }
}

inline void paintVary (Bar& b, const Bar& alt)
{
    if (alt.empty()) return;
    int beat = 1;
    for (auto& n : alt) if (n.step / 4 > 0) { beat = n.step / 4; break; }
    // prefer a beat the alternate actually uses, cycling away from beat 0
    for (int k = 1; k <= 3; ++k) if (std::any_of (alt.begin(), alt.end(), [&] (const Hn& n) { return n.step / 4 == k; })) { beat = k; break; }
    b.erase (std::remove_if (b.begin(), b.end(), [&] (const Hn& n) { return n.step / 4 == beat; }), b.end());
    for (auto& n : alt) if (n.step / 4 == beat) b.push_back (n);
    sortBar (b);
}
} // namespace detail

inline float slideChance (int style, float movement)
{
    switch (style)
    {
        case Acid: return 0.62f;
        case SpeedGarage: return 0.70f;
        case TwoStep: return 0.55f;
        case DeepTech: return 0.48f;
        case Bassline: return 0.22f;
        case Afro: return 0.12f;
        default: return 0.06f * movement;
    }
}

inline std::vector<Note> generate (const Params& P)
{
    const int nb = std::max (1, std::min (16, P.bars));
    const float v = std::max (0.f, std::min (1.f, P.variation));
    detail::Bar hook = detail::buildHook (P, 0u);
    if (hook.empty()) detail::add (hook, 2, 2, 'R');

    std::mt19937 fr (P.rhythmSeed * 97531u + (uint32_t) P.style * 13u + 7u);
    const int smallId = (int) ((fr() + P.take) % 5u);
    const int bigId = (int) ((fr() + P.take) % 4u);
    const detail::Bar small = detail::fill (P.style, false, smallId);
    const detail::Bar large = detail::fill (P.style, true, bigId);

    int shift[4]; barRoots (P, shift);
    auto barShift = [&] (int step) { return shift[std::min (3, (step / 16) % std::min (nb, 4))]; };

    std::vector<Note> out;
    for (int bar = 0; bar < nb; ++bar)
    {
        const detail::Role role = detail::roleOf (bar, nb, v);
        const int phrase = bar / 4;
        detail::Bar b = hook;
        if (detail::longStyle (P.style))
            detail::paintPitch (b, role, phrase, P.take);
        else if (role == detail::Vary)
        {
            detail::Bar alt = detail::buildHook (P, 1000u + (uint32_t) phrase * 17u + P.take * 29u);
            detail::paintVary (b, alt);
        }
        else if (role == detail::Answer || role == detail::Turn)
            detail::paintFill (b, role == detail::Turn ? large : small, P.take);

        for (auto& n : b)
        {
            Note o; o.step = bar * 16 + n.step; o.len = n.len; o.deg = roleDeg (n.r) + barShift (o.step); o.vel = 100;
            out.push_back (o);
        }
    }
    std::sort (out.begin(), out.end(), [] (const Note& a, const Note& c) { return a.step < c.step; });
    out.erase (std::unique (out.begin(), out.end(), [] (const Note& a, const Note& c) { return a.step == c.step; }), out.end());
    const int total = nb * 16;
    for (size_t i = 0; i + 1 < out.size(); ++i)
        out[i].len = std::max (1, std::min (out[i].len, out[i + 1].step - out[i].step));
    if (! out.empty()) out.back().len = std::max (1, std::min (out.back().len, total - out.back().step));

    // walk into the next chord, on the last strong note of the bar
    for (int bar = 0; bar < nb && nb > 1; ++bar)
    {
        const int cyc = std::min (nb, 4);
        const int nextShift = shift[std::min (3, ((bar + 1) % cyc))];
        const int thisShift = shift[std::min (3, bar % cyc)];
        if (nextShift == thisShift) continue;
        Note* last = nullptr;
        for (auto& n : out) if (n.step / 16 == bar) last = &n;
        if (last == nullptr || last->step % 16 < 8) continue;
        const int from = last->deg;
        int a = nextShift + (from > nextShift ? 1 : -1);
        while (a - from > 4) a -= 7;
        while (from - a > 4) a += 7;
        last->deg = a;
        last->vel = std::min (127, last->vel + 6);
    }

    // slides before ghosts, so a glide holds through the gap and the ghost won't sit inside it.
    // long styles may reach across a short rest; acid only glides when the notes already touch.
    const float sp = slideChance (P.style, P.movement);
    const bool reach = P.style == TwoStep || P.style == SpeedGarage || P.style == DeepTech;
    for (size_t i = 1; i < out.size(); ++i)
    {
        auto& n = out[i]; auto& p = out[i - 1];
        if (n.deg == p.deg || std::abs (n.deg - p.deg) > 7) continue;
        const int gap = n.step - (p.step + std::max (p.len, 1));
        if (gap < 0 || gap > (reach ? 4 : 0)) continue;
        // a slide may hold across a rest inside the bar, but it does not reach into the next bar,
        // so a repeated hook stays the same hook
        const bool sameBar = n.step / 16 == p.step / 16;
        if (! sameBar && gap > 0) continue;
        const float h = hash01 (P.rhythmSeed ^ (uint32_t) (n.step % 16) * 101u ^ (uint32_t) std::abs (n.deg - p.deg) * 19u ^ (uint32_t) P.style * 7u);
        if (h < sp)
        {
            n.slide = true;
            if (sameBar) p.len = std::max (1, n.step - p.step);
        }
    }

    // ghosts are a function of the bar position, so a copied bar keeps its ghosts
    if (P.ghosts > 0.01f && ! out.empty())
    {
        std::vector<char> taken ((size_t) total, 0);
        for (auto& n : out) for (int s = n.step; s < std::min (total, n.step + std::max (1, n.len)); ++s) if (s >= 0) taken[(size_t) s] = 1;
        std::vector<Note> addG;
        for (auto& n : out)
        {
            const int s = n.step - 1;
            if (s < 0 || taken[(size_t) s]) continue;
            if (isKickStep (P.style, s) && P.style != Acid) continue;
            const float h = hash01 (P.rhythmSeed ^ (uint32_t) (s % 16) * 97u ^ (uint32_t) (n.deg + 32) * 13u ^ (uint32_t) P.style * 17u);
            if (h < P.ghosts * 0.72f)
            {
                Note g; g.step = s; g.len = 1; g.ghost = true;
                const bool same = hash01 (P.rhythmSeed ^ (uint32_t) (s % 16) * 3u) < 0.65f;
                const int root = barShift (n.step);
                g.deg = same ? n.deg : root + (n.deg >= root + 7 ? 0 : 7);
                const float hv = hash01 (P.rhythmSeed ^ (uint32_t) (s % 16) * 51u);
                g.vel = 46 + (int) std::lround (hv * 11.f); // 46–57
                addG.push_back (g); taken[(size_t) s] = 1;
            }
        }
        out.insert (out.end(), addG.begin(), addG.end());
        std::sort (out.begin(), out.end(), [] (const Note& a, const Note& c) { return a.step < c.step || (a.step == c.step && a.ghost && ! c.ghost); });
        for (size_t i = 0; i + 1 < out.size(); ++i)
            out[i].len = std::max (1, std::min (out[i].len, out[i + 1].step - out[i].step));
    }

    // accents: offbeats speak, acid leans on the beat, and a repeated bar stays identical
    for (auto& n : out)
    {
        if (n.ghost) continue;
        const int pos = n.step % 4;
        int base = pos == 2 ? 116 : pos == 0 ? 104 : pos == 3 ? 96 : 88;
        if (P.style == Acid && pos == 0) base = 124;
        if (P.style == Acid && pos == 2) base = 118;
        const float acc = 0.35f + 1.15f * std::max (0.f, std::min (1.f, P.accent));
        const int wob = (int) (hash01 (P.rhythmSeed ^ (uint32_t) (n.step % 16) * 29u) * 5.f) - 2;
        n.vel = std::clamp ((int) (100 + (base - 100) * acc) + wob, 58, 127);
    }

    if (P.mutations > 0 && ! out.empty())
    {
        std::mt19937 mr (P.mutateSeed ? P.mutateSeed : 1u);
        static const char roles[] = { 'R', 'O', '5', '7', '3' };
        int done = 0;
        for (int tries = 0; done < P.mutations * 2 && tries < 200; ++tries)
        {
            auto& n = out[mr() % out.size()];
            if (n.ghost) continue;
            if (mr() % 3 == 0) { n.vel = std::clamp (n.vel + 8 + (int) (mr() % 17) * ((mr() & 1) ? 1 : -1), 58, 127); ++done; }
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
