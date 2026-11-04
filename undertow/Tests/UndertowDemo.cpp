// UndertowDemo outDir: plays a few styles through the real plugin, saves audio + one UI frame per video frame
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdlib>
struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 128;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setIsPlaying (true); p.setPpqPosition (ppq); return p; }
};
static void setP (UndertowProcessor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
int main (int, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File out (argv[1]); out.getChildFile ("frames").createDirectory();
    const double sr = 48000, fps = 30; const int block = 64;
    UndertowProcessor p; Head h; p.setPlayHead (&h); p.prepareToPlay (sr, block);
    std::unique_ptr<UndertowEditor> ed (dynamic_cast<UndertowEditor*> (p.createEditor()));
    // presets shown through the real preset system, then a bassline drawn by hand
    const int presetsShown[] = { 0, 4, 8, 9, 10, 7, -1 };
    const int nSeg = 7; const double segBeats = 8; // 2 bars each
    const int len = (int) (nSeg * segBeats * 60.0 / h.bpm * sr);
    juce::AudioBuffer<float> all (2, len), buf (2, block); juce::MidiBuffer m;
    int frame = 0, seg = -1;
    // the hand-drawn line: step, degree, length
    const int drawn[][3] = { { 2, 0, 1 }, { 3, 0, 1 }, { 6, 7, 1 }, { 7, 0, 1 }, { 10, 0, 1 }, { 11, 0, 1 }, { 13, 4, 1 }, { 14, 6, 2 },
                             { 18, 5, 1 }, { 19, 5, 1 }, { 22, 12, 1 }, { 23, 5, 1 }, { 26, 5, 1 }, { 27, 5, 1 }, { 29, 4, 1 }, { 30, 1, 2 } };
    std::vector<ut::Note> hand; int nextDraw = 0;
    for (int pos = 0; pos < len; pos += block)
    {
        const int s = (int) (h.ppq / segBeats);
        if (s != seg && s < nSeg)
        {
            seg = s;
            if (presetsShown[s] >= 0) { p.applyPreset (presetsShown[s]); setP (p, "key", 9); setP (p, "bars", 1); if (s > 0) p.generateNew(); p.pump(); }
            else { p.beginEdit(); hand.clear(); p.applyEdit (hand); nextDraw = 0; }
        }
        if (presetsShown[std::min (seg, nSeg - 1)] < 0)
        {
            const double segPos = h.ppq - seg * segBeats; // beats into the segment
            while (nextDraw < 16 && segPos >= nextDraw * 0.42)
            {
                ut::Note n; n.step = drawn[nextDraw][0]; n.deg = drawn[nextDraw][1]; n.len = drawn[nextDraw][2]; n.vel = 108;
                hand.push_back (n); p.applyEdit (hand); ++nextDraw;
            }
        }
        const int n = juce::jmin (block, len - pos);
        buf.setSize (2, n, false, false, true); p.processBlock (buf, m);
        for (int c = 0; c < 2; ++c) all.copyFrom (c, pos, buf, c, 0, n);
        h.ppq += n * h.bpm / 60.0 / sr;
        while (pos / sr >= frame / fps)
        {
            ed->timerCallback();
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = out.getChildFile ("frames/f" + juce::String (frame).paddedLeft ('0', 5) + ".png"); f.deleteFile();
            juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os); ++frame;
        }
    }
    juce::WavAudioFormat wav; auto wf = out.getChildFile ("demo.wav"); wf.deleteFile();
    if (auto os = wf.createOutputStream())
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0))) { os.release(); w->writeFromAudioSampleBuffer (all, 0, len); }
    std::printf ("frames %d\n", frame);
}
