#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

namespace ui
{
// ---- palette ---------------------------------------------------------------
namespace col
{
    const juce::Colour bg       { 0xff120d1c };
    const juce::Colour shell    { 0xff1e1730 };
    const juce::Colour shellHi  { 0xff2c2244 };
    const juce::Colour edge     { 0xff3d2f5e };
    const juce::Colour screenA  { 0xff5b1ee0 };
    const juce::Colour screenB  { 0xff1c0a5c };
    const juce::Colour accent   { 0xffb46cff };
    const juce::Colour accentHi { 0xffe2b8ff };
    const juce::Colour pink     { 0xffff3ec8 };
    const juce::Colour text     { 0xffeee4ff };
    const juce::Colour dim      { 0xff8c7aad };
    const juce::Colour knob     { 0xff2a2140 };
}

struct Fonts
{
    static Fonts& get();
    juce::Typeface::Ptr logo, mono, monoBold;
    juce::Font logoFont (float h) const;
    juce::Font monoFont (float h, bool bold = false) const;
};

juce::String formatValue (const juce::RangedAudioParameter& p, float v);

// ---- look and feel ---------------------------------------------------------
class GlazeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GlazeLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    juce::Font getLabelFont (juce::Label&) override;
};

// ---- knob with label + attachment -----------------------------------------
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label, bool big = false);
    void resized() override;
    void paint (juce::Graphics&) override;
    juce::Slider slider;
private:
    juce::String label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
};

// ---- pill / power toggles --------------------------------------------------
class PillToggle : public juce::ToggleButton
{
public:
    PillToggle (juce::AudioProcessorValueTreeState&, const juce::String& id, juce::String onText, juce::String offText);
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    juce::String onT, offT;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

class PowerButton : public juce::ToggleButton
{
public:
    PowerButton (juce::AudioProcessorValueTreeState&, const juce::String& id);
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
};

class ParamCombo : public juce::ComboBox
{
public:
    ParamCombo (juce::AudioProcessorValueTreeState&, const juce::String& id);
private:
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
};

// ---- screens ---------------------------------------------------------------
class Screen : public juce::Component
{
public:
    explicit Screen (GlazeProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    virtual void tick() {}
    virtual void drawContent (juce::Graphics&, juce::Rectangle<float>) {}
protected:
    float param (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void caption (juce::Graphics&, juce::Rectangle<float>, const juce::String& left, const juce::String& right = {});
    GlazeProcessor& proc;
};

// Rolling history helper
struct History
{
    explicit History (int n = 240, float init = 0.0f) : v ((size_t) n, init) {}
    void push (float x) { v[(size_t) head] = x; head = (head + 1) % (int) v.size(); }
    float operator[] (int i) const { return v[(size_t) ((head + i) % (int) v.size())]; } // 0 = oldest
    int size() const { return (int) v.size(); }
    std::vector<float> v; int head = 0;
};

// Spectrum analyser fed from the output scope
class Spectrum
{
public:
    Spectrum();
    void update (const glaze::ScopeBuffer&, double sr);
    float dbAt (float hz) const;
    void draw (juce::Graphics&, juce::Rectangle<float>, juce::Colour, float minDb = -90, float maxDb = 0) const;
private:
    static constexpr int order = 11, size = 1 << order;
    juce::dsp::FFT fft { order };
    juce::dsp::WindowingFunction<float> window { (size_t) size, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> data, smooth;
    double sr = 44100;
};

float freqToX (float hz, juce::Rectangle<float> r);
float xToFreq (float x, juce::Rectangle<float> r);

class HomeScreen;  class TuneScreen; class GateScreen; class EqScreen; class CompScreen; class DeEssScreen;
class DriveScreen; class StereoScreen; class DelayScreen; class VerbScreen; class LimitScreen;

// ---- page ------------------------------------------------------------------
struct KnobSpec { const char* id; const char* label; };

class Page : public juce::Component
{
public:
    Page (GlazeProcessor&, std::unique_ptr<Screen>, const char* powerId, const juce::String& title, const juce::String& subtitle,
          std::vector<KnobSpec> right, std::vector<KnobSpec> bottom);
    void resized() override;
    void paint (juce::Graphics&) override;
    void addExtra (std::unique_ptr<juce::Component> c, int height) { addAndMakeVisible (*c); extras.push_back ({ std::move (c), height }); }
    std::unique_ptr<Screen> screen;
    std::unique_ptr<PowerButton> power;
    std::vector<std::unique_ptr<Knob>> rightKnobs, bottomKnobs;
    std::vector<std::pair<std::unique_ptr<juce::Component>, int>> extras;
    juce::String title, subtitle;
    bool homeLayout = false;
};

// ---- tab bar ---------------------------------------------------------------
class TabBar : public juce::Component
{
public:
    TabBar (GlazeProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    std::function<void (int)> onSelect;
    int current = 0, hover = -1;
    struct Tab { juce::String name; const char* powerId; };
    std::vector<Tab> tabs;
private:
    int indexAt (juce::Point<int>) const;
    std::vector<juce::Rectangle<float>> bounds() const;
    GlazeProcessor& proc;
};
} // namespace ui

// ============================================================================
class GlazeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GlazeEditor (GlazeProcessor&);
    ~GlazeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    // used by the offline snapshot tool
    void showPageForTest (int i) { showPage (i); }
    void tickForTest() { timerCallback(); }
    int getNumPages() const { return (int) pages.size(); }
private:
    void timerCallback() override;
    void showPage (int);
    void syncPresetBox();

    GlazeProcessor& proc;
    ui::GlazeLookAndFeel lnf;
    ui::TabBar tabs;
    std::vector<std::unique_ptr<ui::Page>> pages;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    std::unique_ptr<ui::PillToggle> lowLatency;
    int current = 0;
    juce::TooltipWindow tooltips { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GlazeEditor)
};
