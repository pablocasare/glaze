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
    int add = 0;        // extra semitones (chord transpose that the scale itself can't spell)
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
    if (style == TwoStep) return step16 == 0 || step16 == 10; // 1 and 11 on a 1-count; the bass answers, it doesn't double them
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

// Shorten a hold so it is silent for `gapMs` before every kick the note would otherwise cover.
// Used by the synth and by MIDI export. gapMs <= 0 leaves the note alone.
inline double applyKickGap (int step, int len, float swing, float gate, int style, bool legato, float bpm, float gapMs)
{
    double off = noteOffBeat (step, len, swing, gate, style, legato);
    if (gapMs <= 0.01f || bpm < 1.f) return off;
    const double on = noteOnBeat (step, swing, style);
    const double gapBeats = (double) gapMs / 1000.0 * ((double) bpm / 60.0);
    const int last = step + std::max (1, len) + 4;
    for (int s = step + 1; s <= last; ++s)
    {
        if (! isKickStep (style, s)) continue;
        const double kb = s * 0.25;
        if (kb > on + 1.0e-4 && kb < off + 1.0e-6)
            off = std::min (off, kb - gapBeats);
    }
    if (off < on + 0.01) off = on + 0.01;
    return off;
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
    int style = Rolling, bars = 2, chords = ChordAuto, scale = 0;
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
    static const std::vector<Cell> twoStep { { { 2, 3, 'R' }, { 6, 2, 'O' }, { 12, 2, '5' }, { 14, 2, '7' } },
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
// 24-bit fraction. Same on libc++, libstdc++ and MSVC. std::uniform_real_distribution is not.
inline float u01 (std::mt19937& g) { return (float) (g() >> 8) * (1.0f / 16777216.0f); }

inline void barRoots (const Params& P, int* shift)
{
    for (int b = 0; b < 4; ++b) shift[b] = 0;
    if (P.bars == 1) return;
    if (P.chords == ChordAuto)
    {
        std::mt19937 hr (P.pitchSeed * 2246822519u + 13u);
        float chance = (P.movement - 0.15f) * 1.15f;
        if (P.style == Minimal || P.style == DeepTech) chance *= 0.35f; // hypnotic styles stay home unless Movement is high
        if (u01 (hr) < chance)
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

inline char rollRole (char r, float m, bool octavey, std::mt19937& g)
{
    // House bass lives on the root, the octave, the 5th and the b7. The 3rd and the 6th only appear
    // when Movement is high, otherwise the line starts to sound like a melody instead of a bassline.
    if (m < 0.08f || u01 (g) > m * 0.42f) return r;
    const bool color = m > 0.72f;
    const float w[] = { 1.15f, octavey ? 1.2f : 0.55f, 0.55f * m + 0.08f, 0.48f * m + 0.08f, color ? 0.18f * m : 0.f, color ? 0.10f * m : 0.f, 0.f, 0.f };
    static const char rs[] = { 'R', 'O', '5', '7', '3', '4', '6', '2' };
    float t = 0; for (float x : w) t += x;
    float x = u01 (g) * t;
    for (int i = 0; i < 8; ++i) { x -= w[i]; if (x <= 0 && w[i] > 0) return rs[i]; }
    return 'R';
}

inline int hookScore (const Bar& b, int style)
{
    if (b.size() < 2) return -200;
    int s = 12, roots = 0, oct = 0, onAnd = 0;
    for (auto& n : b)
    {
        if (n.r == 'R') ++roots;
        else if (n.r == 'O') { ++roots; ++oct; }
        else if (n.r == '5' || n.r == '7') s += 1;
        else s -= 6;
        if ((n.step % 4) == 2) ++onAnd;
        const bool kickOk = style == Acid || style == TwoStep || style == Broken || style == Afro;
        if (! kickOk && isKickStep (style, n.step)) s -= 12;
    }
    s += roots * 3 + oct;
    if (style == Offbeat || style == Bassline || style == DeepTech) s += onAnd * 4;
    if (style == Bouncy || style == SpeedGarage || style == Bassline) s += oct * 3;
    if (style == Driving && (int) b.size() >= 8) s += 6;
    return s;
}

inline bool longStyle (int s)
{
    return s == TwoStep || s == SpeedGarage || s == DeepTech || s == Minimal || s == Afro;
}

// One bar of onsets. Mandatory steps are the style's identity (the "&", the tresillo).
// The pool is shuffled by the rhythm seed and density takes the first k, so the same
// knob position is a different groove per seed, and raising density only adds notes.
inline void composeBar (Bar& b, const Params& P, std::mt19937& rr, float d)
{
    const bool houseKick = P.style != Acid && P.style != Afro && P.style != Broken;
    std::vector<int> must, pool;
    auto push = [&] (std::vector<int>& v, int s)
    {
        if (s < 0 || s > 15) return;
        if (houseKick && isKickStep (P.style, s)) return;
        if (P.style == Broken && isKickStep (Broken, s)) return;
        for (int x : v) if (x == s) return;
        for (int x : must) if (x == s) return;
        v.push_back (s);
    };
    switch (P.style)
    {
        case Offbeat:
            for (int s : { 2, 6, 10, 14 }) push (must, s);
            if (d >= 0.22f) for (int s : { 3, 7, 11, 15, 1, 5, 9, 13 }) push (pool, s);
            break;
        case Driving:
            for (int s : { 2, 6, 10, 14 }) push (must, s);
            if (d >= 0.22f) for (int s : { 3, 7, 11, 15 }) push (pool, s);
            if (d >= 0.55f) for (int s : { 1, 5, 9, 13 }) push (pool, s);
            break;
        case Afro:
            for (int s : { 0, 3, 6 }) push (must, s);
            for (int s : { 8, 10, 11, 14, 2, 5, 9, 12, 15, 13, 7, 4 }) push (pool, s);
            break;
        case Bassline:
            for (int s : { 2, 6, 10, 14 }) push (must, s);
            for (int s : { 3, 7, 11, 15, 1, 5, 9, 13 }) push (pool, s);
            break;
        case TwoStep: // eight off-grid answers; none of them is kick 0 or kick 10
            for (int s : { 1, 3, 5, 7, 9, 11, 13, 15 }) push (pool, s);
            break;
        case SpeedGarage: case DeepTech: case Minimal:
            for (int s : { 1, 3, 5, 7, 9, 11, 13, 15 }) push (pool, s);
            break;
        default:
            for (int s = 0; s < 16; ++s) push (pool, s);
            break;
    }
    int budget = pool.empty() ? 0 : (int) std::lround (std::clamp (d, 0.f, 1.f) * (float) pool.size());
    budget = std::max (0, std::min ((int) pool.size(), budget));
    for (int i = (int) pool.size() - 1; i > 0; --i)
    {
        const int j = (int) (rr() % (uint32_t) (i + 1));
        std::swap (pool[(size_t) i], pool[(size_t) j]);
    }
    std::vector<int> steps = must;
    const bool sustain = P.style == TwoStep || P.style == SpeedGarage || P.style == DeepTech || P.style == Minimal;
    for (int i = 0; i < budget; ++i) steps.push_back (pool[(size_t) i]);
    if (steps.empty()) steps.push_back (houseKick || P.style == TwoStep ? 2 : 0);
    if (steps.size() < 2)
    {
        const int s0 = steps[0];
        for (int alt : { s0 + 4, s0 + 6, s0 + 8, s0 - 4, 6, 10, 14, 2, 5, 9 })
        {
            int t = alt % 16; if (t < 0) t += 16;
            if (houseKick && isKickStep (P.style, t)) continue;
            if (P.style == TwoStep && (t == 0 || t == 10)) continue;
            if (P.style == Broken && isKickStep (Broken, t)) continue;
            bool exists = false; for (int x : steps) if (x == t) exists = true;
            if (! exists) { steps.push_back (t); break; }
        }
    }
    std::sort (steps.begin(), steps.end());
    steps.erase (std::unique (steps.begin(), steps.end()), steps.end());
    for (size_t i = 0; i < steps.size(); ++i)
    {
        int len = 1;
        if (sustain)
        {
            const int next = i + 1 < steps.size() ? steps[i + 1] : 16;
            len = std::max (2, next - steps[i]);
        }
        add (b, steps[i], len, 'R');
    }
}

inline Bar buildHook (const Params& P, uint32_t salt)
{
    std::mt19937 rr (P.rhythmSeed * 2654435761u + (uint32_t) P.style * 97u + salt * 131u);
    std::mt19937 pr (P.pitchSeed * 40503u + 7u + salt * 17u);
    const float d = std::max (0.f, std::min (1.f, P.density));
    const float m = std::max (0.f, std::min (1.f, P.movement));
    Bar b;
    composeBar (b, P, rr, d);

    const bool octavey = P.style == Bouncy || P.style == Bassline || P.style == SpeedGarage;
    const float mEff = m * (P.style == Minimal || P.style == DeepTech || P.style == Offbeat ? 0.35f
                            : P.style == Driving || P.style == Afro ? 0.45f
                            : P.style == Bassline || P.style == SpeedGarage || P.style == Rolling ? 0.4f : 0.7f);
    if (P.style != Bassline && P.style != SpeedGarage && P.style != Afro && P.style != Acid)
        for (auto& n : b) n.r = rollRole (n.r, mEff, octavey, pr);
    else if (m > 0.15f)
        for (size_t i = 1; i < b.size(); ++i) if (u01 (pr) < mEff * 0.45f) b[i].r = rollRole (b[i].r, mEff, octavey, pr);

    sortBar (b);
    if (! b.empty() && P.style != Bassline && P.style != SpeedGarage && P.style != Bouncy) b.front().r = 'R';
    if (P.style == Afro)
        for (auto& n : b) if (n.step == 0) n.r = 'R';
    if (! b.empty() && m > 0.12f && (P.style == Rolling || P.style == Driving || P.style == Offbeat || P.style == Bouncy) && b.back().step != b.front().step)
    {
        static const char cad[] = { 'R', 'O', '7', '5' };
        b.back().r = cad[(P.pitchSeed + salt) % (m > 0.6f ? 4u : 3u)];
    }
    return b;
}

inline Bar fill (int style, bool big, int index)
{
    // Endings stay in the style. A shared 5-7-3 run is what made every genre sound like the same fill.
    static const Bar rollS[] = {
        { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 2, 1, 'R' }, { 3, 1, '7' } }, { { 2, 2, 'O' } },
        { { 1, 1, 'R' }, { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 2, 1, 'O' }, { 3, 1, '7' } } };
    static const Bar rollB[] = {
        { { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, '7' } },
        { { 2, 1, '7' }, { 3, 1, '5' } }, { { 1, 1, 'O' }, { 2, 1, '5' }, { 3, 1, '7' } } };
    static const Bar bounce[] = {
        { { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 1, 1, 'O' }, { 2, 1, 'R' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } } };
    static const Bar drive[] = {
        { { 2, 1, 'O' }, { 3, 1, '7' } }, { { 2, 1, 'R' }, { 3, 1, 'O' } }, { { 1, 1, 'R' }, { 2, 1, 'O' }, { 3, 1, '7' } } };
    static const Bar broken[] = {
        { { 2, 1, '5' }, { 3, 1, '7' } }, { { 2, 1, 'O' }, { 3, 1, 'R' } }, { { 0, 1, 'R' }, { 2, 1, '7' }, { 3, 1, '5' } } };
    static const Bar acid[] = {
        { { 0, 1, 'O' }, { 1, 1, '7' }, { 2, 1, '5' }, { 3, 1, 'R' } },
        { { 0, 1, 'R' }, { 1, 1, 'O' }, { 2, 1, '7' }, { 3, 1, '5' } },
        { { 1, 1, 'O' }, { 2, 1, '5' }, { 3, 1, '7' } } };
    if (style == Acid) return acid[index % 3];
    if (style == Bouncy) return bounce[index % 3];
    if (style == Driving) return drive[index % 3];
    if (style == Broken) return broken[index % 3];
    return big ? rollB[index % 4] : rollS[index % 5];
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
    static const char choices[] = { '7', '5', 'O', 'R' };
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
        static const char o[] = { '7', '5', 'O' }; // a leading tone back to the root, not a random 3rd
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

// Pitches come only from the pitch seed, so Lock Pitch keeps them when the rhythm changes
// and Lock Rhythm still gets new notes.
inline char pitchRole (const Params& P, int index)
{
    const uint32_t h = mix32 (P.pitchSeed * 747796405u + (uint32_t) index * 2891336453u + (uint32_t) P.style * 97u);
    const float u = (h >> 8) * (1.0f / 16777216.0f);
    const float m = std::max (0.f, std::min (1.f, P.movement));
    if (index == 0 && P.style != Bouncy && P.style != Bassline && P.style != SpeedGarage) return 'R';
    if (P.style == Bassline)
    {
        if (index == 1)
        {
            static const char o[] = { 'O', 'R', '7', '5' };
            return o[mix32 (P.pitchSeed + 3u) % 4];
        }
        return 'O';
    }
    if (P.style == SpeedGarage)
    {
        if (index == 1)
        {
            static const char o[] = { 'R', 'O', '7', '5' };
            return o[mix32 (P.pitchSeed + 9u) % 4];
        }
        return (index & 1) ? 'R' : 'O';
    }
    if (P.style == Bouncy)
    {
        static const char o[] = { 'O', 'R', 'O', 'R', 'O', '7', 'R', 'O' };
        return o[h % 8];
    }
    if (P.style == Acid)
    {
        static const char o[] = { 'R', 'R', 'O', '5', '7', 'R', 'O', '5' };
        return o[h % 8];
    }
    if (u > 0.22f + m * 0.55f) return 'R';
    static const char p[] = { 'O', '5', '7', 'R' };
    return p[h % 4];
}
inline void assignHookPitch (detail::Bar& hook, const Params& P)
{
    std::sort (hook.begin(), hook.end(), [] (const detail::Hn& a, const detail::Hn& c) { return a.step < c.step; });
    for (size_t i = 0; i < hook.size(); ++i) hook[i].r = pitchRole (P, (int) i);
}
inline int chordSemis (int shiftDeg)
{
    // Menu labels are minor roman numerals, so the interval is the natural-minor one in every scale.
    return degToSemi (shiftDeg, Minor);
}

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
    // Several hooks from this seed, keep the one that sits in the style. Same seeds always pick the same one.
    // Salt 0 is the seed's own hook. Only replace it when another take is clearly stronger,
    // so Generate stays varied instead of collapsing onto one "highest scoring" grid.
    // Salt 0 only. A search that scores pitched hooks would let the pitch seed
    // change the rhythm, and Lock Rhythm would not hold.
    detail::Bar hook = detail::buildHook (P, 0u);
    if (hook.empty()) detail::add (hook, 2, 2, 'R');
    assignHookPitch (hook, P);
    if (P.mutations > 0 && hook.size() > 1)
    {
        // Edit the hook itself, so every repeat of it changes together. Always a pitch, never velocity-only.
        const uint32_t h = mix32 ((P.mutateSeed ? P.mutateSeed : 1u) + (uint32_t) P.mutations * 97u);
        const int idx = 1 + (int) (h % (uint32_t) (hook.size() - 1));
        static const char roles[] = { 'O', '5', '7', 'R' };
        char nr = roles[h % 4];
        if (nr == hook[(size_t) idx].r) nr = roles[(h + 1) % 4];
        hook[(size_t) idx].r = nr;
    }

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
        // Offbeat and bassline keep their grid. The answer is a different pitch, not a chopped 16th fill.
        const bool gentle = detail::longStyle (P.style) || P.style == Offbeat || P.style == Bassline;
        if (gentle)
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
            Note o; o.step = bar * 16 + n.step; o.len = n.len; o.vel = 100;
            const int sh = barShift (o.step);
            if (P.scale == MinorPent || P.scale == Major || P.scale == HarmMinor)
            {
                o.deg = roleDeg (n.r);
                o.add = chordSemis (sh);
            }
            else o.deg = roleDeg (n.r) + sh;
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
        const int wob = (int) (hash01 (P.pitchSeed ^ (uint32_t) (n.step % 16) * 29u) * 5.f) - 2;
        n.vel = std::clamp ((int) (100 + (base - 100) * acc) + wob, 58, 127);
    }
    // A new pitch seed is always a new pattern, including styles whose roles are almost fixed.
    // Applied once per bar so a repeated hook stays identical.
    for (int bar = 0; bar < nb; ++bar)
    {
        Note* first = nullptr; Note* second = nullptr;
        for (auto& n : out)
        {
            if (n.ghost || n.step / 16 != bar) continue;
            if (first == nullptr) first = &n;
            else { second = &n; break; }
        }
        if (Note* t = second ? second : first) t->vel = std::clamp (64 + (int) (P.pitchSeed % 41u), 40, 127);
    }

    return out;
}
} // namespace ut
