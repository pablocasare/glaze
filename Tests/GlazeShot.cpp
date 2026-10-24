// Renders each UI page to PNG while feeding synthetic vocal audio through the processor.
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? argv[1] : juce::File::getCurrentWorkingDirectory().getFullPathName());
    GlazeProcessor proc;
    const double sr = 48000; const int block = 256;
    proc.setRateAndBufferSizeDetails (sr, block);
    proc.prepareToPlay (sr, block);
    proc.setCurrentProgram (0);
    proc.apvts.getParameter ("gateOn")->setValueNotifyingHost (1.0f);
    proc.apvts.getParameter ("drvOn")->setValueNotifyingHost (1.0f);
    proc.apvts.getParameter ("chOn")->setValueNotifyingHost (1.0f);

    std::unique_ptr<GlazeEditor> ed (dynamic_cast<GlazeEditor*> (proc.createEditor()));
    juce::AudioBuffer<float> buf (2, block); juce::MidiBuffer midi;
    double t = 0, ph = 0; juce::Random rng (1);
    auto run = [&] (double seconds)
    {
        const int blocks = (int) (seconds * sr / block);
        const int perTick = (int) (sr / 30 / block);
        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < block; ++i, t += 1.0 / sr)
            {
                // sung phrase: notes with vibrato, slightly off pitch, gaps, and "s" noise bursts
                const double phrase = std::fmod (t, 2.4);
                const int note = phrase < 0.7 ? 57 : (phrase < 1.4 ? 60 : 62);
                const double f = 440.0 * std::pow (2.0, (note - 69 + 0.3 * std::sin (t * 1.3) + 0.25 * std::sin (2 * M_PI * 5.5 * t)) / 12.0);
                ph += 2 * M_PI * f / sr;
                double s = 0; for (int h = 1; h < 16; ++h) s += std::sin (h * ph) / h * (1 + 2 * std::exp (-std::pow ((h * f - 800) / 300, 2)));
                double env = phrase < 2.0 ? std::min (1.0, phrase * 20) * std::min (1.0, (2.0 - phrase) * 10) : 0.0;
                double noise = (phrase > 2.0 && phrase < 2.25) ? (rng.nextFloat() * 2 - 1) * 0.15 : 0.0;
                const float x = (float) (0.12 * s * env + noise);
                buf.setSample (0, i, x); buf.setSample (1, i, x);
            }
            proc.processBlock (buf, midi);
            if (b % perTick == 0) ed->tickForTest();
        }
    };
    const char* names[] { "home", "tune", "gate", "eq", "comp", "deess", "drive", "double", "chorus", "delay", "verb", "limit" };
    for (int p = 0; p < ed->getNumPages(); ++p)
    {
        ed->showPageForTest (p);
        run (p == 0 ? 3.0 : 1.3);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        auto f = outDir.getChildFile (juce::String ("page_") + juce::String (p).paddedLeft ('0', 2) + "_" + names[p] + ".png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::printf ("wrote %s\n", f.getFullPathName().toRawUTF8());
    }
    ed.reset();
    return 0;
}
