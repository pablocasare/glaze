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
    struct Seg { int style, key, scale, sound; float swing; int gen; };
    const Seg segs[] = { { 0, 9, 0, 1, 0.56f, 0 }, { 0, 9, 0, 1, 0.56f, 1 }, { 3, 9, 0, 2, 0.64f, 1 }, { 7, 9, 2, 4, 0.54f, 1 }, { 2, 9, 1, 3, 0.55f, 1 }, { 5, 9, 0, 5, 0.53f, 1 } };
    const int nSeg = 6; const double segBeats = 8; // 2 bars each
    const int len = (int) (nSeg * segBeats * 60.0 / h.bpm * sr);
    juce::AudioBuffer<float> all (2, len), buf (2, block); juce::MidiBuffer m;
    int frame = 0, seg = -1;
    for (int pos = 0; pos < len; pos += block)
    {
        const int s = (int) (h.ppq / segBeats);
        if (s != seg && s < nSeg)
        {
            seg = s; auto& g = segs[s];
            setP (p, "style", (float) g.style); setP (p, "key", (float) g.key); setP (p, "scale", (float) g.scale); setP (p, "sound", (float) g.sound); setP (p, "swing", g.swing); if (std::getenv ("NOKICK")) setP (p, "kick", 0);
            if (g.gen) p.generateNew(); else p.regenerate();
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
