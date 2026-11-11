// Full-screen host for the website demo. Shows the real editor and records the
// real synth, in time with the display, to a wav the recorder muxes later.
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <cmath>
#include <sys/time.h>

struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 126;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setIsPlaying (true); p.setPpqPosition (ppq); return p;
    }
};

static void setP (UndertowProcessor& p, const char* id, float v)
{
    if (auto* q = p.apvts.getParameter (id))
        q->setValueNotifyingHost (q->convertTo0to1 (v));
}

class Recorder : public juce::Thread
{
public:
    Recorder (UndertowProcessor& p, Head& h, juce::AudioFormatWriter* w)
        : juce::Thread ("ut-rec"), proc (p), head (h), writer (w) {}

    void run() override
    {
        const double sr = 48000.0;
        struct timeval tv;
        gettimeofday (&tv, nullptr);
        std::printf ("AUDIO %ld.%06ld\n", (long) tv.tv_sec, (long) tv.tv_usec);
        std::fflush (stdout);
        const double beatSamp = sr * 60.0 / head.bpm;
        auto t0 = juce::Time::getMillisecondCounterHiRes();
        juce::int64 written = 0;
        float phase = 0;
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer midi;
        while (! threadShouldExit())
        {
            const double elapsed = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
            const juce::int64 need = (juce::int64) (elapsed * sr);
            if (written >= need) { wait (2); continue; }
            const int n = (int) juce::jmin<juce::int64> (256, need - written);
            buf.setSize (2, n, false, false, true);
            midi.clear();
            proc.processBlock (buf, midi);
            for (int i = 0; i < n; ++i)
            {
                const double into = std::fmod ((double) (written + i), beatSamp);
                const float t = (float) (into / sr);
                float k = 0;
                if (t < 0.16f)
                {
                    const float env = std::exp (-t * 18.f);
                    const float f = 42.f + 130.f * std::exp (-t * 34.f);
                    phase += juce::MathConstants<float>::twoPi * f / (float) sr;
                    const float click = t < 0.0025f ? (1.f - t / 0.0025f) * 0.35f : 0.f;
                    k = (std::sin (phase) * 0.9f + click) * env;
                }
                for (int c = 0; c < 2; ++c)
                {
                    const float x = buf.getSample (c, i) * 0.78f + k * 0.38f;
                    buf.setSample (c, i, std::tanh (x));
                }
            }
            writer->writeFromAudioSampleBuffer (buf, 0, n);
            head.ppq += n * head.bpm / 60.0 / sr;
            written += n;
        }
    }

private:
    UndertowProcessor& proc;
    Head& head;
    juce::AudioFormatWriter* writer;
};

class Stage : public juce::DocumentWindow, private juce::Timer
{
public:
    Stage (UndertowProcessor& p, Recorder& rec) : DocumentWindow ("", juce::Colour (0xff121417), 0), proc (p), audio (rec)
    {
        setUsingNativeTitleBar (false);
        setTitleBarHeight (0);
        setDropShadowEnabled (false);
        setResizable (false, false);
        holder = new juce::Component();
        holder->setSize (1920, 1080);
        editor.reset (dynamic_cast<UndertowEditor*> (proc.createEditor()));
        editor->setResizable (false, false);
        holder->addAndMakeVisible (*editor);
        editor->setBounds (143, 0, 1634, 1080);
        setContentOwned (holder, true);
        setBounds (0, 0, 1920, 1080);
        setVisible (true);
        toFront (true);
        startTimerHz (20);
        std::printf ("READY\n");
        std::fflush (stdout);
    }

    void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

    void timerCallback() override
    {
        auto req = juce::File ("/tmp/ut-shot-req");
        if (req.existsAsFile())
        {
            const auto name = req.loadFileAsString().trim();
            req.deleteFile();
            if (name.isNotEmpty() && editor != nullptr)
            {
                const float shown = (float) editor->getWidth() / 1180.f;
                auto img = editor->createComponentSnapshot (editor->getLocalBounds(), true, 2.f / shown);
                auto out = juce::File ("/tmp/ut-shots").getChildFile (name + ".png");
                out.deleteFile();
                juce::FileOutputStream os (out);
                juce::PNGImageFormat().writeImageToStream (img, os);
                std::printf ("SHOT %s %d %d\n", name.toRawUTF8(), img.getWidth(), img.getHeight());
                std::fflush (stdout);
            }
        }
        if (juce::File ("/tmp/ut-stage-quit").existsAsFile())
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }

private:
    UndertowProcessor& proc;
    Recorder& audio;
    juce::Component* holder = nullptr;
    std::unique_ptr<UndertowEditor> editor;
};

class App : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Undertow"; }
    const juce::String getApplicationVersion() override { return "2.0.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }

    void initialise (const juce::String&) override
    {
        juce::File ("/tmp/ut-shots").createDirectory();
        juce::File ("/tmp/ut-stage-quit").deleteFile();
        juce::File ("/tmp/ut-shot-req").deleteFile();
        proc = std::make_unique<UndertowProcessor>();
        proc->setPlayHead (&head);
        proc->prepareToPlay (48000, 256);
        setP (*proc, "sound", 0.f);   // Sub, a sine
        setP (*proc, "evolve", 0.f);  // the roll stays until we click
        setP (*proc, "kick", 0.f);
        setP (*proc, "playMode", 0.f);
        proc->pump();
        wavFile = juce::File ("/tmp/ut-stage.wav");
        wavFile.deleteFile();
        stream = wavFile.createOutputStream();
        writer.reset (wav.createWriterFor (stream.get(), 48000, 2, 24, {}, 0));
        if (writer != nullptr) stream.release();
        rec = std::make_unique<Recorder> (*proc, head, writer.get());
        rec->startThread (juce::Thread::Priority::high);
        window = std::make_unique<Stage> (*proc, *rec);
    }

    void shutdown() override
    {
        if (rec != nullptr) { rec->signalThreadShouldExit(); rec->stopThread (2000); }
        window.reset();
        writer.reset();
        if (proc != nullptr) proc->releaseResources();
        proc.reset();
    }

private:
    Head head;
    std::unique_ptr<UndertowProcessor> proc;
    juce::WavAudioFormat wav;
    juce::File wavFile;
    std::unique_ptr<juce::FileOutputStream> stream;
    std::unique_ptr<juce::AudioFormatWriter> writer;
    std::unique_ptr<Recorder> rec;
    std::unique_ptr<Stage> window;
};

START_JUCE_APPLICATION (App)
