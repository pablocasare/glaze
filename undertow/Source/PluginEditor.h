#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace utui
{
namespace col
{
    const juce::Colour bg { 0xff0a0b0d }, panel { 0xff131418 }, panel2 { 0xff1a1c21 }, line { 0xff25282f }, lane { 0xff16181c },
        lime { 0xffcef25c }, limeDim { 0xff5f7a26 }, cream { 0xfff0f8d6 }, text { 0xfff4f4f6 }, grey { 0xff8c8f98 }, faint { 0xff4a4d55 };
}
struct Fonts
{
    juce::Typeface::Ptr black, bold, medium, mono;
    Fonts();
    juce::Font b (float h) const { return juce::Font (juce::FontOptions (black).withHeight (h)); }
    juce::Font s (float h) const { return juce::Font (juce::FontOptions (bold).withHeight (h)); }
    juce::Font m (float h) const { return juce::Font (juce::FontOptions (medium).withHeight (h)); }
    juce::Font n (float h) const { return juce::Font (juce::FontOptions (mono).withHeight (h)); }
};

struct LnF : juce::LookAndFeel_V4
{
    const Fonts& f;
    explicit LnF (const Fonts& fonts);
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return f.s (14); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override { return f.s (15); }
    void drawPopupMenuBackground (juce::Graphics&, int, int) override;
};

struct Knob : juce::Component
{
    juce::Slider s { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::String label; const Fonts& f;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    Knob (const Fonts& fonts, juce::AudioProcessorValueTreeState& st, const char* id, juce::String lab);
    void resized() override { s.setBounds (getLocalBounds().withTrimmedTop (18).withTrimmedBottom (16)); }
    void paint (juce::Graphics&) override;
};

struct Chip : juce::Button
{
    const Fonts& f; juce::String sub; bool filled = true;
    Chip (const Fonts& fonts, const juce::String& t) : juce::Button (t), f (fonts) {}
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

struct Roll : juce::Component
{
    UndertowProcessor& p; const Fonts& f;
    std::vector<ut::Note> pat; int version = -1;
    float flash[256] {};
    Roll (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) {}
    void paint (juce::Graphics&) override;
    void refresh();
};

struct DragTile : juce::Component
{
    UndertowProcessor& p; const Fonts& f; bool dragging = false;
    DragTile (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
};
}

class UndertowEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit UndertowEditor (UndertowProcessor&);
    ~UndertowEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    utui::Roll& getRoll() { return roll; }

private:
    UndertowProcessor& proc;
    utui::Fonts fonts;
    utui::LnF lnf { fonts };
    utui::Roll roll { proc, fonts };
    std::vector<std::unique_ptr<utui::Chip>> styles;
    std::vector<std::unique_ptr<utui::Knob>> knobs;
    juce::ComboBox key, scale, octave, bars, sound, chords;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    utui::Chip generate { fonts, "GENERATE" }, mutate { fonts, "MUTATE" }, undo { fonts, "UNDO" }, play { fonts, "" }, kick { fonts, "KICK" },
        lockR { fonts, "RHYTHM" }, lockP { fonts, "PITCH" }, exportB { fonts, "EXPORT" };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btns;
    utui::DragTile drag { proc, fonts };
    std::unique_ptr<juce::FileChooser> chooser;
    int lastStyle = -1;
};
