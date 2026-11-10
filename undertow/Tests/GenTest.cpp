// Generator invariants. No JUCE: c++ -std=c++17 -O2 -o /tmp/gentest Tests/GenTest.cpp && /tmp/gentest
#include "../Source/Gen.h"
#include <cstdio>
#include <cmath>
#include <string>
#include <map>
#include <set>

static int failures = 0;
static void check (bool ok, const char* msg)
{
    std::printf ("%s %s\n", ok ? "PASS" : "FAIL", msg);
    if (! ok) ++failures;
}

static ut::Params base (int style, float density, float variation, int bars = 4)
{
    ut::Params P;
    P.style = style; P.bars = bars; P.chords = ut::ChordStay;
    P.density = density; P.movement = 0.45f; P.variation = variation;
    P.gate = 0.8f; P.ghosts = 0.3f; P.accent = 0.5f;
    P.rhythmSeed = 11; P.pitchSeed = 7; P.mutations = 0; P.take = 0;
    return P;
}

// Rhythm, pitch, length, velocity and ghosts. Slide is allowed to differ on the first note of a bar,
// because that glide comes from whatever the previous bar ended on.
static std::string body (const std::vector<ut::Note>& v, int bar, int maxPos = 16)
{
    std::string s;
    for (auto& n : v) if (n.step / 16 == bar && (n.step % 16) < maxPos)
        s += std::to_string (n.step % 16) + ":" + std::to_string (n.deg) + ":" + std::to_string (n.len) + ":" + std::to_string (n.vel) + (n.ghost ? "g" : "") + " ";
    return s;
}

// The note that leads into beat 4 may hold longer when that beat is a fill, so length is compared
// on the repeated hook only. The first three beats must still be the same pitches.
static std::string pitches (const std::vector<ut::Note>& v, int bar, int maxPos = 16)
{
    std::string s;
    for (auto& n : v) if (! n.ghost && n.step / 16 == bar && (n.step % 16) < maxPos)
        s += std::to_string (n.step % 16) + ":" + std::to_string (n.deg) + " ";
    return s;
}

int main()
{
    // determinism
    {
        auto P = base (ut::Rolling, 0.6f, 0.3f);
        auto a = ut::generate (P), b = ut::generate (P);
        bool same = a.size() == b.size();
        for (size_t i = 0; same && i < a.size(); ++i)
            same = a[i].step == b[i].step && a[i].deg == b[i].deg && a[i].len == b[i].len && a[i].vel == b[i].vel && a[i].slide == b[i].slide && a[i].ghost == b[i].ghost;
        check (same, "same seeds, same pattern");
    }

    // every pattern is playable: unique steps, in range, no stuck lengths, degrees sane
    {
        int bad = 0, empty = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 40; ++seed)
            {
                auto P = base (s, 0.15f + 0.8f * ((seed % 5) / 4.f), (seed % 7) / 6.f, 1 + (seed % 5 == 0 ? 0 : seed % 5 == 1 ? 1 : 3));
                P.rhythmSeed = (uint32_t) (seed * 17 + 3); P.pitchSeed = (uint32_t) (seed * 29 + 1);
                P.movement = (seed % 4) / 3.f; P.ghosts = (seed % 3) / 2.f; P.chords = seed % ut::NumChords;
                auto v = ut::generate (P);
                if ((int) v.size() < 2) { ++empty; continue; }
                for (size_t i = 0; i < v.size(); ++i)
                {
                    if (v[i].step < 0 || v[i].step >= P.bars * 16 || v[i].len < 1) ++bad;
                    if (i + 1 < v.size() && v[i].step >= v[i + 1].step) ++bad;
                    if (i + 1 < v.size() && v[i].step + v[i].len > v[i + 1].step) ++bad;
                    if (v[i].vel < 30 || v[i].vel > 127) ++bad;
                    if (std::abs (v[i].deg) > 21) ++bad;
                }
            }
        check (bad == 0 && empty == 0, "notes in range, no overlaps, no empty patterns");
        if (bad || empty) std::printf ("   bad %d empty %d\n", bad, empty);
    }

    // variation 0 locks the hook: bars 1 and 2 match bar 1's content, bar 4 only changes the ending
    {
        int hookFail = 0, bodyFail = 0, n = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 30; ++seed)
            {
                auto P = base (s, 0.2f + 0.6f * ((seed % 4) / 3.f), 0.0f, 4);
                P.rhythmSeed = (uint32_t) (1000 + seed * 13 + s * 50); P.pitchSeed = (uint32_t) (50 + seed * 9);
                P.ghosts = 0.35f; P.movement = 0.4f;
                auto v = ut::generate (P); ++n;
                if (body (v, 0) != body (v, 1)) ++hookFail;
                if (pitches (v, 0, 12) != pitches (v, 2, 12) || pitches (v, 0, 12) != pitches (v, 3, 12)) ++bodyFail;
            }
        check (hookFail == 0, "variation 0: bar 1 equals bar 2 (hook, ghosts, accents)");
        check (bodyFail == 0, "variation 0: the first three beats of every bar match");
        if (hookFail || bodyFail) std::printf ("   hook mismatches %d  body mismatches %d  of %d\n", hookFail, bodyFail, n);
    }

    // default variation is call and response: bar 1 == bar 3, and it is not all four bars the same
    {
        int same = 0, changed = 0, n = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 20; ++seed)
            {
                auto P = base (s, 0.6f, 0.30f, 4);
                P.rhythmSeed = (uint32_t) (seed * 19 + s * 7 + 4); P.pitchSeed = (uint32_t) (seed * 3 + 9);
                auto v = ut::generate (P); ++n;
                if (body (v, 0, 12) == body (v, 2, 12)) ++same;
                if (body (v, 0) != body (v, 1)) ++changed;
            }
        check (same == n, "variation 0.3: bar 1 returns as bar 3");
        check (changed > n * 8 / 10, "variation 0.3: bar 2 answers bar 1");
        if (same != n || changed <= n * 8 / 10) std::printf ("   returned %d answered %d of %d\n", same, changed, n);
    }

    // evolve keeps the hook and moves the answer
    {
        int kept = 0, moved = 0, n = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 8; ++seed)
            {
                auto P = base (s, 0.6f, 0.30f, 4);
                P.rhythmSeed = (uint32_t) (seed + 20 + s); P.pitchSeed = (uint32_t) (seed + 4);
                auto a = ut::generate (P); P.take = 1; auto b = ut::generate (P); ++n;
                if (body (a, 0) == body (b, 0)) ++kept;
                if (body (a, 1) != body (b, 1) || body (a, 3) != body (b, 3)) ++moved;
            }
        check (kept == n, "evolve keeps bar 1");
        check (moved == n, "evolve changes the answer");
        if (kept != n || moved != n) std::printf ("   kept %d moved %d of %d\n", kept, moved, n);
    }

    auto houseKick = [] (int s) { return s != ut::Acid && s != ut::TwoStep && s != ut::Broken && s != ut::Afro; };
    {
        int onKick = 0, notes = 0;
        for (int s = 0; s < ut::NumStyles; ++s) if (houseKick (s))
            for (int seed = 0; seed < 25; ++seed)
            {
                auto P = base (s, (seed % 3) / 2.f, (seed % 3) * 0.4f, 4);
                P.rhythmSeed = (uint32_t) (seed * 11 + s); P.pitchSeed = 3;
                for (auto& n : ut::generate (P))
                {
                    if (n.ghost) continue;
                    ++notes;
                    if (ut::isKickStep (s, n.step)) ++onKick;
                }
            }
        check (notes > 0 && onKick == 0, "house styles keep note-ons off the kick");
        if (notes) std::printf ("   on-kick %d / %d (%.1f%%)\n", onKick, notes, 100.0 * onKick / notes);
    }

    // offbeat at a normal density is the "&"
    {
        int onAnd = 0, notes = 0;
        for (int seed = 0; seed < 30; ++seed)
        {
            auto P = base (ut::Offbeat, 0.45f, 0.1f, 4);
            P.rhythmSeed = (uint32_t) (seed * 5 + 2); P.pitchSeed = (uint32_t) seed;
            for (auto& n : ut::generate (P)) if (! n.ghost && n.step < 32)
            { ++notes; if ((n.step % 4) == 2) ++onAnd; }
        }
        check (notes > 0 && onAnd * 100 / notes >= 90, "offbeat: the and of the beat");
        if (notes) std::printf ("   on-and %d / %d\n", onAnd, notes);
    }

    // driving thins, and the "e" is the first thing to go
    {
        auto low = base (ut::Driving, 0.0f, 0.0f, 4);
        auto high = low; high.density = 1.f;
        int eLow = 0, eHigh = 0, nLow = 0, nHigh = 0;
        for (int seed = 0; seed < 20; ++seed)
        {
            low.rhythmSeed = high.rhythmSeed = (uint32_t) (seed + 1);
            low.pitchSeed = high.pitchSeed = (uint32_t) (seed + 8);
            for (auto& n : ut::generate (low)) if (! n.ghost && n.step < 16) { ++nLow; if ((n.step % 4) == 1) ++eLow; }
            for (auto& n : ut::generate (high)) if (! n.ghost && n.step < 16) { ++nHigh; if ((n.step % 4) == 1) ++eHigh; }
        }
        check (nHigh >= nLow + 4 * 20 && eLow == 0 && eHigh > 20, "driving: density adds the e, low density drops it");
        std::printf ("   low %d (e %d)  high %d (e %d) per 20 bars\n", nLow, eLow, nHigh, eHigh);
    }

    auto meanLen = [] (int style, float density) {
        double sum = 0; int n = 0;
        for (int seed = 0; seed < 25; ++seed)
        {
            auto P = base (style, density, 0.25f, 4);
            P.rhythmSeed = (uint32_t) (seed * 8 + style); P.pitchSeed = 11; P.ghosts = 0.1f;
            for (auto& x : ut::generate (P)) if (! x.ghost) { sum += x.len; ++n; }
        }
        return n ? sum / n : 0;
    };
    {
        const double ts = meanLen (ut::TwoStep, 0.6f), sg = meanLen (ut::SpeedGarage, 0.6f), dt = meanLen (ut::DeepTech, 0.6f);
        check (ts >= 2.0, "2-step notes are long");
        check (sg >= 2.2, "speed garage holds for a Reese");
        check (dt >= 2.2, "deep tech subs are long");
        std::printf ("   mean len  2-step %.2f  speed %.2f  deep %.2f sixteenths\n", ts, sg, dt);
    }
    {
        int slides = 0, pairs = 0;
        for (int seed = 0; seed < 20; ++seed)
        {
            auto P = base (ut::Acid, 0.7f, 0.3f, 4);
            P.rhythmSeed = (uint32_t) seed; P.pitchSeed = 4; P.movement = 0.6f;
            auto v = ut::generate (P);
            for (size_t i = 1; i < v.size(); ++i) if (! v[i].ghost && ! v[i - 1].ghost && v[i].deg != v[i - 1].deg) { ++pairs; slides += v[i].slide; }
        }
        check (pairs > 0 && slides * 100 / pairs >= 25, "acid slides");
        std::printf ("   acid slides %d / %d pitch changes\n", slides, pairs);
        int slides2 = 0, pairs2 = 0;
        for (int seed = 0; seed < 20; ++seed)
        {
            auto P = base (ut::TwoStep, 0.6f, 0.3f, 4);
            P.rhythmSeed = (uint32_t) (seed + 3); P.pitchSeed = 6;
            auto v = ut::generate (P);
            for (size_t i = 1; i < v.size(); ++i) if (! v[i].ghost && ! v[i - 1].ghost && v[i].deg != v[i - 1].deg) { ++pairs2; slides2 += v[i].slide; }
        }
        check (pairs2 > 0 && slides2 * 100 / pairs2 >= 20, "2-step slides");
        std::printf ("   2-step slides %d / %d\n", slides2, pairs2);
    }

    // afro keeps the tresillo anchor in bar 1
    {
        int anchored = 0;
        for (int seed = 0; seed < 30; ++seed)
        {
            auto P = base (ut::Afro, 0.55f, 0.2f, 4);
            P.rhythmSeed = (uint32_t) (seed + 1); P.pitchSeed = (uint32_t) (seed * 2);
            std::set<int> steps;
            for (auto& n : ut::generate (P)) if (! n.ghost && n.step < 16) steps.insert (n.step);
            if (steps.count (0) && steps.count (3) && steps.count (6)) ++anchored;
        }
        check (anchored == 30, "afro: tresillo 0, 3, 6 in every hook");
    }

    // bassline default is offbeat octaves (degree 7)
    {
        int oct = 0, notes = 0;
        for (int seed = 0; seed < 20; ++seed)
        {
            auto P = base (ut::Bassline, 0.6f, 0.0f, 2);
            P.rhythmSeed = (uint32_t) seed; P.pitchSeed = 2; P.movement = 0.2f; P.ghosts = 0;
            for (auto& n : ut::generate (P)) if (n.step < 16) { ++notes; if ((n.step % 4) == 2 && (n.deg % 7 + 7) % 7 == 0 && n.deg != 0) ++oct; }
        }
        check (notes > 0 && oct * 100 / std::max (1, notes) >= 50, "bassline: offbeat octaves");
        std::printf ("   octave offbeats %d / %d\n", oct, notes);
    }

    // pentatonic degrees only use pentatonic semitones, mixolydian has the b7
    {
        bool pent = true, mix = true;
        auto P = base (ut::Rolling, 0.8f, 0.8f, 4); P.movement = 1.f; P.rhythmSeed = 99; P.pitchSeed = 99;
        for (auto& n : ut::generate (P))
        {
            const int semi = ut::degToSemi (n.deg, ut::MinorPent);
            const int pc = (semi % 12 + 12) % 12;
            if (pc != 0 && pc != 3 && pc != 5 && pc != 7 && pc != 10) pent = false;
        }
        const int b7 = ut::degToSemi (6, ut::Mixolydian);
        mix = b7 == 10;
        check (pent && mix, "minor pentatonic stays in the five notes, mixolydian has a b7");
    }

    // timing: swing delays the off-16ths, the house "&" is a touch early, legato overlaps
    {
        const double straight = ut::noteOnBeat (2, 0.50f, ut::Driving);
        const double early = ut::noteOnBeat (2, 0.50f, ut::Offbeat);
        const double swung = ut::noteOnBeat (1, 0.62f, ut::TwoStep);
        const double grid = ut::noteOnBeat (1, 0.50f, ut::TwoStep);
        const double off = ut::noteOffBeat (0, 4, 0.5f, 1.f, ut::DeepTech, false);
        const double leg = ut::noteOffBeat (0, 4, 0.5f, 0.2f, ut::DeepTech, true);
        check (early < straight && swung > grid && leg > off, "swing, pocket and legato timing");
    }

    if (failures)
    {
        auto P = base (ut::Rolling, 0.6f, 0.0f, 4);
        auto v = ut::generate (P);
        std::printf ("-- rolling variation 0 sample --\n");
        for (int bar = 0; bar < 4; ++bar) std::printf ("bar %d  %s\n", bar + 1, body (v, bar).c_str());
    }
    std::printf (failures ? "%d FAILURES\n" : "ALL OK\n", failures);
    return failures ? 1 : 0;
}
