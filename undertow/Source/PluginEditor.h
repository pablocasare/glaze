#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

namespace utui
{
namespace col
{
    // graphite hardware, one signal colour
    const juce::Colour bg { 0xff121417 }, chassis { 0xff181b1f }, module { 0xff1d2125 }, moduleHi { 0xff23282d }, edge { 0xff2c3137 }, edgeDark { 0xff0a0b0d },
        screen { 0xff0b0d0f }, screenLine { 0xff1a1e22 }, screenBeat { 0xff22272c }, screenBar { 0xff323940 },
        sig { 0xffb8ff3c }, sigDim { 0xff6d9a22 }, sigDeep { 0xff1c2e0c },
        ink { 0xffe8eaed }, label { 0xff8a919a }, dim { 0xff5a6169 }, faint { 0xff3a4046 }, warn { 0xffe7a04a };
}
struct Fonts
{
    juce::Typeface::Ptr black, bold, medium, mono;
    Fonts();
    juce::Font b (float h) const { return juce::Font (juce::FontOptions (black).withHeight (h)); }
    juce::Font s (float h) const { return juce::Font (juce::FontOptions (bold).withHeight (h)); }
    juce::Font m (float h) const { return juce::Font (juce::FontOptions (medium).withHeight (h)); }
    juce::Font n (float h) const { return juce::Font (juce::FontOptions (mono).withHeight (h)); }
    juce::Font caps (float h) const { auto f = s (h); f.setExtraKerningFactor (0.14f); return f; }
};

struct LnF : juce::LookAndFeel_V4
{
    const Fonts& f;
    explicit LnF (const Fonts& fonts);
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return f.n (12.5f); }
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override { return f.m (14); }
    void drawPopupMenuBackground (juce::Graphics&, int, int) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool, bool, bool, bool, bool, const juce::String&, const juce::String&,
                            const juce::Drawable*, const juce::Colour*) override;
};

struct Knob : juce::Component
{
    juce::Slider s { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::String label; const Fonts& f;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    Knob (const Fonts& fonts, juce::AudioProcessorValueTreeState& st, const char* id, juce::String lab);
    void resized() override { s.setBounds (getLocalBounds().withTrimmedTop (16).withTrimmedBottom (18)); }
    void paint (juce::Graphics&) override;
};

// flat hardware button, optional LED for toggles
struct Btn : juce::Button
{
    enum Kind { Plain, Toggle, Primary };
    const Fonts& f; Kind kind; juce::String icon;
    Btn (const Fonts& fonts, const juce::String& t, Kind k = Plain) : juce::Button (t), f (fonts), kind (k) { if (k == Toggle) setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

struct StyleList : juce::Component, public juce::SettableTooltipClient
{
    UndertowProcessor& p; const Fonts& f; int hover = -1;
    StyleList (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) {}
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent& e) override
    {
        const int h = rowAt (e.y);
        if (h >= 0) setTooltip (juce::String (ut::styleName (h)) + "  —  " + ut::styleBlurb (h) + "   Try " + juce::String (ut::styleBpm (h)) + " BPM");
        if (h != hover) { hover = h; repaint(); }
    }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override;
    int rowAt (int y) const { return juce::jlimit (-1, ut::NumStyles - 1, y / rowH()); }
    int rowH() const { return getHeight() / ut::NumStyles; }
};

struct PresetBar : juce::Component
{
    UndertowProcessor& p; const Fonts& f;
    PresetBar (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
};

struct Roll : juce::Component, public juce::SettableTooltipClient
{
    UndertowProcessor& p; const Fonts& f;
    std::vector<ut::Note> pat; int version = -1;
    float flash[2048] {};
    int hoverNote = -1;
    float zoom = 1.f; // 1, 2 or 4. Wheel scrolls, ctrl-wheel zooms.
    int scroll = 0;
    int viewSteps() const { return std::max (16, (int) std::lround ((p.bars() * 16) / std::max (1.f, zoom))); }
    // edit state
    enum Drag { None, Move, Resize, Velocity } drag = None;
    int dragIdx = -1, grabStepOff = 0; ut::Note orig;
    int lo = -3, hi = 12;                 // visible scale-degree range
    Roll (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) { setRepaintsOnMouseActivity (false); }
    void paint (juce::Graphics&) override;
    void refresh();
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    // geometry
    juce::Rectangle<float> gridArea() const;
    juce::Rectangle<float> velArea() const;
    float xOfStep (double step) const;
    int stepAt (float x) const;
    int degAt (float y) const;
    juce::Rectangle<float> noteRect (const ut::Note&) const;
    int noteAt (juce::Point<float>) const;
    void computeRange();
};

struct DragTile : juce::Component, public juce::SettableTooltipClient
{
    UndertowProcessor& p; const Fonts& f; bool dragging = false;
    DragTile (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override { dragging = false; }
};

struct SlotBar : juce::Component
{
    UndertowProcessor& p; const Fonts& f; int hover = -1; static constexpr int LW = 46;
    int slotAt (int x) const { return juce::jlimit (0, UndertowProcessor::NumSlots - 1, (int) ((x - LW) / ((getWidth() - LW) / (float) UndertowProcessor::NumSlots))); }
    SlotBar (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent& e) override { hover = e.x < LW ? -1 : slotAt (e.x); repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
};

struct Scope : juce::Component
{
    UndertowProcessor& p; const Fonts& f;
    Scope (UndertowProcessor& proc, const Fonts& fonts) : p (proc), f (fonts) {}
    void paint (juce::Graphics&) override;
};

// everything lives here at 1x; the editor scales it
struct Content : juce::Component
{
    static constexpr int W = 1180, H = 780;
    UndertowProcessor& proc; const Fonts& fonts;
    juce::Image grain;
    StyleList styles; PresetBar preset; Roll roll; DragTile drag; Scope scope; SlotBar slotBar { proc, fonts };
    std::vector<std::unique_ptr<Knob>> knobs;
    juce::ComboBox key, scale, octave, bars, chords, sound, playMode;
    juce::Slider kickGap { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gapAtt;
    Btn generate { fonts, "GENERATE", Btn::Primary }, mutate { fonts, "VARY" }, undo { fonts, "UNDO" }, redo { fonts, "REDO" },
        play { fonts, "", Btn::Plain }, kick { fonts, "KICK", Btn::Toggle }, lockR { fonts, "RHYTHM", Btn::Toggle }, lockP { fonts, "PITCH", Btn::Toggle },
        exportB { fonts, "EXPORT" }, clear { fonts, "CLEAR" }, evolve { fonts, "EVOLVE", Btn::Toggle }, keyIn { fonts, "MIDI KEY", Btn::Toggle };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> btns;
    std::unique_ptr<juce::FileChooser> chooser;
    Content (UndertowProcessor&, const Fonts&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void module (juce::Graphics&, juce::Rectangle<float>, const char* title) const;
    void tick();
};
}

class UndertowEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit UndertowEditor (UndertowProcessor&);
    ~UndertowEditor() override;
    void resized() override;
    void timerCallback() override;
    utui::Roll& getRoll() { return content.roll; }
    utui::Content& getContent() { return content; }

private:
    UndertowProcessor& proc;
    utui::Fonts fonts;
    utui::LnF lnf { fonts };
    juce::TooltipWindow tips;
    utui::Content content { proc, fonts };
};
