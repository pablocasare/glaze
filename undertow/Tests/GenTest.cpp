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
            auto P = base (ut::Offbeat, 0.15f, 0.1f, 4);
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
        check (notes > 0 && oct >= 20 * 3, "bassline: offbeat octaves");
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

    auto rhythmOf = [] (const std::vector<ut::Note>& v) {
        std::string s;
        for (auto& n : v) if (! n.ghost && n.step < 16) s += std::to_string (n.step) + ",";
        return s;
    };
    auto sameNotes = [] (const std::vector<ut::Note>& a, const std::vector<ut::Note>& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (a[i].step != b[i].step || a[i].len != b[i].len || a[i].deg != b[i].deg || a[i].vel != b[i].vel || a[i].ghost != b[i].ghost)
                return false;
        return true;
    };

    // defaults: density 0.58, movement 0.28, variation 0.16 — at least 40 bar-1 rhythms / 300 seeds
    {
        int worst = 999;
        for (int s = 0; s < ut::NumStyles; ++s)
        {
            std::set<std::string> rh;
            for (int seed = 0; seed < 300; ++seed)
            {
                auto P = base (s, 0.58f, 0.16f, 4);
                P.movement = 0.28f; P.ghosts = 0.f;
                P.rhythmSeed = (uint32_t) (seed * 97 + 11); P.pitchSeed = (uint32_t) (seed * 13 + 3);
                rh.insert (rhythmOf (ut::generate (P)));
            }
            if ((int) rh.size() < worst) worst = (int) rh.size();
            check ((int) rh.size() >= 40, "variety: >= 40 bar-1 rhythms / 300 seeds");
            if ((int) rh.size() < 40) std::printf ("   style %d rhythms %d\n", s, (int) rh.size());
        }
        std::printf ("   fewest bar-1 rhythms %d\n", worst);
    }

    // 21 density steps, >= 8 distinct bar-1 rhythms for every style (one seed, and the mean)
    {
        int worst = 999;
        for (int s = 0; s < ut::NumStyles; ++s)
        {
            int sum = 0;
            for (int seed = 0; seed < 12; ++seed)
            {
                std::set<std::string> rh;
                for (int step = 0; step <= 20; ++step)
                {
                    auto P = base (s, step / 20.f, 0.16f, 1);
                    P.movement = 0.28f; P.ghosts = 0.f;
                    P.rhythmSeed = (uint32_t) (100 + seed * 17 + s); P.pitchSeed = 5;
                    rh.insert (rhythmOf (ut::generate (P)));
                }
                sum += (int) rh.size();
            }
            const int avg = sum / 12;
            if (avg < worst) worst = avg;
            check (avg >= 8, "density: >= 8 distinct outputs over 21 steps");
            if (avg < 8) std::printf ("   style %d density states %d\n", s, avg);
        }
        std::printf ("   fewest density states %d\n", worst);
    }

    // Lock Rhythm: same rhythm seed, new pitch seed, never the same pattern. Onsets stay.
    {
        int identical = 0, onsetDrift = 0, n = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 40; ++seed)
            {
                auto P = base (s, 0.58f, 0.16f, 2);
                P.movement = 0.28f; P.ghosts = 0.f; P.chords = ut::ChordStay;
                P.rhythmSeed = (uint32_t) (seed * 19 + s * 3 + 1);
                P.pitchSeed = (uint32_t) (seed + 8);
                auto a = ut::generate (P);
                P.pitchSeed = (uint32_t) (seed + 9);
                auto b = ut::generate (P);
                ++n;
                if (sameNotes (a, b)) ++identical;
                if (rhythmOf (a) != rhythmOf (b)) ++onsetDrift;
            }
        check (identical == 0, "lock rhythm: a new pitch seed never returns the same pattern");
        check (onsetDrift == 0, "lock rhythm: onsets stay put");
        if (identical || onsetDrift) std::printf ("   identical %d onset drift %d of %d\n", identical, onsetDrift, n);
    }

    // Lock Pitch: same pitch seed, new rhythm. The role sequence (degrees) of the shared prefix matches.
    {
        int broken = 0, n = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 30; ++seed)
            {
                auto P = base (s, 0.58f, 0.0f, 1);
                P.movement = 0.2f; P.ghosts = 0.f; P.chords = ut::ChordStay;
                P.pitchSeed = (uint32_t) (50 + s * 9);
                P.rhythmSeed = (uint32_t) (seed * 23 + 4);
                auto a = ut::generate (P);
                P.rhythmSeed = (uint32_t) (seed * 23 + 900);
                auto b = ut::generate (P);
                std::vector<int> da, db;
                for (auto& nte : a) if (! nte.ghost) da.push_back (nte.deg);
                for (auto& nte : b) if (! nte.ghost) db.push_back (nte.deg);
                const size_t m = std::min (da.size(), db.size());
                ++n;
                if (m < 2) { ++broken; continue; }
                for (size_t i = 0; i < m; ++i) if (da[i] != db[i]) { ++broken; break; }
            }
        check (broken == 0, "lock pitch: role sequence survives a new rhythm");
        if (broken) std::printf ("   broken %d of %d\n", broken, n);
    }

    // Vary edits the hook: pitch or onset changes, and bar 1 still equals bar 3.
    {
        int unchanged = 0, hookBreak = 0, n = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 20; ++seed)
            {
                auto P = base (s, 0.58f, 0.0f, 4);
                P.movement = 0.28f; P.ghosts = 0.f;
                P.rhythmSeed = (uint32_t) (seed * 11 + s); P.pitchSeed = 17; P.mutateSeed = (uint32_t) (seed + 3);
                auto before = ut::generate (P);
                P.mutations = 1;
                auto after = ut::generate (P);
                ++n;
                bool pitchOrOnset = before.size() != after.size();
                const size_t m = std::min (before.size(), after.size());
                for (size_t i = 0; i < m && ! pitchOrOnset; ++i)
                    if (before[i].step != after[i].step || before[i].deg != after[i].deg) pitchOrOnset = true;
                if (! pitchOrOnset) ++unchanged;
                if (pitches (after, 0) != pitches (after, 2)) ++hookBreak;
            }
        check (unchanged == 0, "vary: every click changes a pitch or an onset");
        check (hookBreak == 0, "vary: the hook still repeats");
        if (unchanged || hookBreak) std::printf ("   unchanged %d hook breaks %d of %d\n", unchanged, hookBreak, n);
    }

    // 2-step bass does not start on kick steps 0 or 10.
    {
        int hit = 0, notes = 0;
        for (int seed = 0; seed < 80; ++seed)
        {
            auto P = base (ut::TwoStep, 0.3f + 0.6f * ((seed % 5) / 4.f), 0.2f, 2);
            P.rhythmSeed = (uint32_t) seed; P.pitchSeed = 4; P.ghosts = 0.4f;
            for (auto& n : ut::generate (P))
            {
                if (n.ghost) continue;
                ++notes;
                const int s = n.step & 15;
                if (s == 0 || s == 10) ++hit;
            }
        }
        check (notes > 0 && hit == 0, "2-step: no bass note-on on steps 0 or 10");
        if (hit) std::printf ("   kick doubles %d / %d\n", hit, notes);
    }

    // Kick gap opens a silence before every kick the note would have covered.
    {
        int through = 0, covered = 0;
        for (int s = 0; s < ut::NumStyles; ++s)
            for (int seed = 0; seed < 15; ++seed)
            {
                auto P = base (s, 0.7f, 0.1f, 2);
                P.rhythmSeed = (uint32_t) (seed + s * 5); P.pitchSeed = 2; P.ghosts = 0.f;
                for (auto& n : ut::generate (P))
                {
                    if (n.ghost) continue;
                    const double on = ut::noteOnBeat (n.step, 0.54f, s);
                    const double off = ut::applyKickGap (n.step, n.len, 0.54f, 0.8f, s, false, 128.f, 12.f);
                    for (int k = n.step; k <= n.step + n.len + 1; ++k)
                    {
                        if (! ut::isKickStep (s, k) || k == n.step) continue;
                        const double kb = k * 0.25;
                        if (kb <= on + 1e-6) continue;
                        ++covered;
                        if (kb < off - 1e-4) ++through;
                    }
                }
            }
        check (covered > 0 && through == 0, "kick gap: no note sounds through a kick");
        if (through) std::printf ("   through %d / %d kicks\n", through, covered);
    }

    // Chord menu intervals are natural-minor semitones, including in pentatonic.
    {
        const int vi = ut::chordSemis (-2), vii = ut::chordSemis (-1), iv = ut::chordSemis (3), V = ut::chordSemis (-3), iii = ut::chordSemis (2);
        check (vi == -4 && vii == -2 && iv == 5 && V == -5 && iii == 3, "chord menu: i-VI -4, i-VII -2, iv +5, V -5, III +3");
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
