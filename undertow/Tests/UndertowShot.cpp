// Offline: UndertowShot outDir  -> screenshots, demo audio, MIDI for several styles
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>

struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 128; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setIsPlaying (playing); p.setPpqPosition (ppq); return p; }
};
static void setP (UndertowProcessor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argc > 1 ? argv[1] : ".");
    out.createDirectory();
    const double sr = 48000; const int block = 256;
    struct Case { const char* name; int style, key, scale, sound; float bpm, swing; int bars; };
    const Case cases[] = { { "rolling", 0, 9, 0, 1, 128, 0.56f, 2 }, { "bouncy", 1, 5, 1, 0, 126, 0.55f, 2 }, { "offbeat", 2, 7, 0, 3, 124, 0.54f, 2 },
                           { "2step", 3, 2, 0, 2, 134, 0.66f, 4 }, { "minimal", 4, 0, 2, 0, 128, 0.58f, 2 }, { "driving", 5, 9, 0, 5, 130, 0.52f, 2 },
                           { "broken", 6, 4, 0, 1, 127, 0.6f, 4 }, { "acid", 7, 9, 2, 4, 130, 0.54f, 2 } };
    int failures = 0;
    for (auto& c : cases)
    {
        UndertowProcessor p; Head h; h.bpm = c.bpm; p.setPlayHead (&h);
        setP (p, "style", (float) c.style); setP (p, "key", (float) c.key); setP (p, "scale", (float) c.scale); setP (p, "sound", (float) c.sound);
        setP (p, "swing", c.swing); setP (p, "bars", c.bars == 1 ? 0.f : (c.bars == 2 ? 1.f : 2.f));
        p.prepareToPlay (sr, block);
        p.regenerate();
        auto pat = p.getPattern();
        std::printf ("%-8s %2d notes:", c.name, (int) pat.size());
        for (auto& n : pat) std::printf (" %d:%d%s", n.step, n.deg, n.slide ? "~" : "");
        std::printf ("\n");
        if (pat.size() < 3) ++failures;
        // render 8 bars
        const int len = (int) (8 * 4 * 60.0 / c.bpm * sr);
        juce::AudioBuffer<float> all (2, len), buf (2, block); juce::MidiBuffer m;
        std::unique_ptr<UndertowEditor> ed (dynamic_cast<UndertowEditor*> (p.createEditor()));
        int midiOn = 0; float pk = 0; bool shot = false;
        for (int pos = 0; pos < len; pos += block)
        {
            const int n = juce::jmin (block, len - pos);
            buf.setSize (2, n, false, false, true);
            p.processBlock (buf, m);
            for (const auto md : m) if (md.getMessage().isNoteOn()) ++midiOn;
            for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, pos, buf, ch, 0, n);
            pk = std::max (pk, buf.getMagnitude (0, n));
            h.ppq += n * c.bpm / 60.0 / sr;
            if (pos % (block * 8) == 0) { ed->getRoll().refresh(); }
            if (! shot && h.ppq > 5.3) // grab a frame mid-bar 2
            {
                for (int k = 0; k < 3; ++k) { ed->timerCallback(); }
                auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
                auto f = out.getChildFile (juce::String ("ui_") + c.name + ".png"); f.deleteFile();
                juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os); shot = true;
            }
        }
        std::printf ("         midi note-ons %d  peak %.2f\n", midiOn, pk);
        if (midiOn < 3 || pk < 0.05f || pk > 1.5f || ! std::isfinite (pk)) ++failures;
        juce::WavAudioFormat wav; auto wf = out.getChildFile (juce::String ("demo_") + c.name + ".wav"); wf.deleteFile();
        if (auto os = wf.createOutputStream())
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0))) { os.release(); w->writeFromAudioSampleBuffer (all, 0, len); }
        p.writeMidi (out.getChildFile (juce::String ("bass_") + c.name + ".mid"));
    }
    // determinism + mutate + undo + state
    {
        UndertowProcessor a; a.regenerate(); auto p1 = a.getPattern();
        a.mutate(); auto p2 = a.getPattern(); a.undo(); auto p3 = a.getPattern();
        bool same = p1.size() == p3.size(); for (size_t i = 0; same && i < p1.size(); ++i) same = p1[i].step == p3[i].step && p1[i].deg == p3[i].deg;
        bool diff = p1.size() != p2.size(); for (size_t i = 0; ! diff && i < p1.size(); ++i) diff = p1[i].deg != p2[i].deg || p1[i].vel != p2[i].vel;
        a.generateNew(); juce::MemoryBlock mb; a.getStateInformation (mb); auto g1 = a.getPattern();
        UndertowProcessor b; b.setStateInformation (mb.getData(), (int) mb.getSize()); auto g2 = b.getPattern();
        bool st = g1.size() == g2.size(); for (size_t i = 0; st && i < g1.size(); ++i) st = g1[i].step == g2[i].step && g1[i].deg == g2[i].deg;
        std::printf ("%s undo restores\n%s mutate changes notes\n%s state recall\n", same ? "PASS" : "FAIL", diff ? "PASS" : "FAIL", st ? "PASS" : "FAIL");
        failures += ! same + ! diff + ! st;
    }
    std::printf (failures ? "%d FAILURES\n" : "ALL OK\n", failures);
    return failures;
}
