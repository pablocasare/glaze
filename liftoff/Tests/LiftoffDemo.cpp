// Offline demo: run a stereo WAV through Liftoff with an automated BUILD curve, save the
// processed audio plus one UI frame per video frame.
//   LiftoffDemo in.wav out.wav framesDir bpm fps  <  automation (lines: seconds build%)
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <vector>

struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 127;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setIsPlaying (true); p.setPpqPosition (ppq); return p; }
};

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 6) { std::fprintf (stderr, "usage\n"); return 1; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (juce::File { juce::String (argv[1]) }));
    if (! rd) { std::fprintf (stderr, "cannot read input\n"); return 1; }
    const double sr = rd->sampleRate; const int len = (int) rd->lengthInSamples;
    juce::AudioBuffer<float> in (2, len); rd->read (&in, 0, len, 0, true, true);
    const double bpm = atof (argv[4]); const double fps = atof (argv[5]);
    std::vector<std::pair<double, double>> pts; double t, v;
    while (std::cin >> t >> v) pts.push_back ({ t, v });
    auto buildAt = [&] (double s)
    {
        if (pts.empty()) return 0.0;
        if (s <= pts.front().first) return pts.front().second;
        for (size_t i = 1; i < pts.size(); ++i)
            if (s < pts[i].first)
            {
                const double a = pts[i - 1].first, b = pts[i].first;
                return b - a < 1e-6 ? pts[i].second : pts[i - 1].second + (pts[i].second - pts[i - 1].second) * (s - a) / (b - a);
            }
        return pts.back().second;
    };

    LiftoffProcessor p; Head h; h.bpm = bpm; p.setPlayHead (&h);
    const int block = 64;
    p.setRateAndBufferSizeDetails (sr, block); p.prepareToPlay (sr, block);
    std::unique_ptr<LiftoffEditor> ed (dynamic_cast<LiftoffEditor*> (p.createEditor()));
    auto* prm = p.apvts.getParameter ("build");
    const juce::File outDir { juce::String (argv[3]) }; outDir.createDirectory();

    juce::AudioBuffer<float> out (2, len), buf (2, block); juce::MidiBuffer m;
    int frame = 0; double nextFrame = 0;
    for (int pos = 0; pos < len; pos += block)
    {
        const int n = juce::jmin (block, len - pos);
        const double sec = pos / sr;
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) buildAt (sec)));
        buf.setSize (2, n, false, false, true);
        for (int c = 0; c < 2; ++c) buf.copyFrom (c, 0, in, c, pos, n);
        p.processBlock (buf, m);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
        h.ppq += n * bpm / 60.0 / sr;
        while (sec >= nextFrame)
        {
            ed->timerCallback();
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
            auto f = outDir.getChildFile ("f" + juce::String (frame).paddedLeft ('0', 5) + ".png");
            f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
            ++frame; nextFrame = frame / fps;
        }
    }
    juce::File of { juce::String (argv[2]) }; of.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = of.createOutputStream())
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)))
        { os.release(); w->writeFromAudioSampleBuffer (out, 0, len); }
    std::printf ("frames %d  peak %.3f  ui %dx%d\n", frame, out.getMagnitude (0, len), ed->getWidth(), ed->getHeight());
    return 0;
}
